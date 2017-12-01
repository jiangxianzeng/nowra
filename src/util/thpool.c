/*
 *
 * */


/* System libraries. */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stddef.h>
#include <errno.h>
#include <time.h>

/* Application-specific. */

#include "thpool.h"
#include "ring.h"
#include "msg.h"
#include "mymalloc.h"

#define MAX_NANOSEC 999999999
#define CEIL(X) ((X-(int)(X)) > 0 ? (int)(X+1) : (int)(X))

static volatile int threads_keepalive;
static volatile int threads_on_hold;


#define RING_TO_JOB_T(r) \
        ((THPOOL_JOB_T *) ((char *) (r) - offsetof(THPOOL_JOB_T, ring)))

typedef struct THPOOL_BSEM {
    pthread_mutex_t mutex;
    pthread_cond_t   cond;
    int v;
}THPOOL_BSEM;

/* Job */
typedef struct THPOOL_JOB_T {
    THPOOL_NOTIFY_F callback; /* function pointer*/
    char *context;
    RING ring;
} THPOOL_JOB_T;

/* Job queue */
typedef struct THPOOL_JOBQUEUE {
    RING job_head;
    int len;                  /* amount of jobs in queue */
    THPOOL_BSEM *queue_sem;     /*  */
    pthread_mutex_t rwmutex;    /* used for queue r/w access */
} THPOOL_JOBQUEUE;

/* Thread */
typedef struct THPOOL_THREAD_T {
    int       id;                        /* friendly id               */
    pthread_t pthread;                   /* pointer to actual thread  */
    struct THPOOL_T* thpool_p;            /* access to thpool          */

} THPOOL_THREAD_T;

/* Threadpool */
struct THPOOL_T {
    THPOOL_THREAD_T **threads; /* pointer to threads' ID  */
    volatile int num_threads_alive;      /* threads currently alive   */
    volatile int num_threads_working;    /* threads currently working */
    pthread_mutex_t  thcount_lock;       /* used for thread count etc */
    THPOOL_JOBQUEUE *jobqueue;
};

static void thpool_bsem_init(THPOOL_BSEM *bsem_p, int value) 
{
    const char *myname = "thpool_bsem_init";

    if (value < 0 || value > 1) {
        msg_panic("%s: bsem_init(): Binary semaphore can take only values 1 or 0", myname);
    }
    pthread_mutex_init(&(bsem_p->mutex), NULL);
    pthread_cond_init(&(bsem_p->cond), NULL);
    bsem_p->v = value;
}

/* Reset semaphore to 0 */
static void thpool_bsem_reset(THPOOL_BSEM *bsem_p) 
{
    thpool_bsem_init(bsem_p, 0);
}

/* Post to at least one thread */
static void thpool_bsem_post(THPOOL_BSEM *bsem_p) 
{
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_signal(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

/* Post to all threads */
static void thpool_bsem_post_all(THPOOL_BSEM *bsem_p) 
{
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_broadcast(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

/* Wait on semaphore until semaphore has value 0 */
static void thpool_bsem_wait(THPOOL_BSEM* bsem_p) 
{
    pthread_mutex_lock(&bsem_p->mutex);
    while (bsem_p->v != 1) {
        pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex);
    }
    bsem_p->v = 0;
    pthread_mutex_unlock(&bsem_p->mutex);
}

/* thpool_jobqueue_init - Initialise job queue */
static int thpool_jobqueue_init(THPOOL_T* tp_p)
{
    const char *myname = "thpool_jobqueue_init";

    if(tp_p == NULL) {
        msg_error("%s: tp_p is NULL.", myname);
        return -1;
    }

    tp_p->jobqueue = (THPOOL_JOBQUEUE *)mymalloc(sizeof(THPOOL_JOBQUEUE));
    if(tp_p->jobqueue == NULL) {
        return -1;
    }
    
    tp_p->jobqueue->queue_sem=(THPOOL_BSEM*)mymalloc(sizeof(THPOOL_BSEM)); /* MALLOC job queue semaphore */
    if(tp_p->jobqueue->queue_sem == NULL) {
        myfree((char *)tp_p->jobqueue);
        return -1;
    }

    pthread_mutex_init(&(tp_p->jobqueue->rwmutex), NULL);
    thpool_bsem_init(tp_p->jobqueue->queue_sem, 0);

    ring_init(&(tp_p->jobqueue->job_head));
    tp_p->jobqueue->len=0;

    return 0;
}

/* Get first job from queue(removes it from queue)
 *  
 * Notice: Caller MUST hold a mutex
 * */
static THPOOL_JOB_T* thpool_jobqueue_pull(THPOOL_T* tp_p) {

    THPOOL_JOB_T *job_p = NULL;
    job_p = RING_TO_JOB_T(RING_SUCC(&(tp_p->jobqueue->job_head)));
    ring_detach(RING_SUCC(&(tp_p->jobqueue->job_head)));

    switch(tp_p->jobqueue->len){

        case 0:  /* if no jobs in queue */
            break;

        case 1:  /* if one job in queue */
            tp_p->jobqueue->len = 0;
            break;

        default: /* if >1 jobs in queue */
            tp_p->jobqueue->len--;
            /* more than one job in queue -> post it */
            thpool_bsem_post(tp_p->jobqueue->queue_sem);
    }
    return job_p;
}

/* Clear the queue */
static void thpool_jobqueue_clear(THPOOL_T* tp_p){

    while(tp_p->jobqueue->len){
        myfree((char *)thpool_jobqueue_pull(tp_p));
    }

    thpool_bsem_reset(tp_p->jobqueue->queue_sem);
    tp_p->jobqueue->len = 0;

}

/* thpool_jobqueue_add - Add job to queue */
static void thpool_jobqueue_push(THPOOL_T* tp_p, THPOOL_JOB_T* newjob_p)
{
    const char *myname = "thpool_jobqueue_push";

    ring_prepend(&(tp_p->jobqueue->job_head), &(newjob_p->ring));

    (tp_p->jobqueue->len)++; /* increment amount of jobs in queue */
    thpool_bsem_post(tp_p->jobqueue->queue_sem);

}

/* Free all queue resources back to the system */
static void thpool_jobqueue_destroy(THPOOL_T* tp_p)
{
    thpool_jobqueue_clear(tp_p);
    myfree((char *)tp_p->jobqueue->queue_sem);
}



/* Sets the calling thread on hold */
static void thread_hold () 
{
    threads_on_hold = 1;
    while (threads_on_hold){
        sleep(1);
    }
}
/* */

static void* thpool_thread_do(THPOOL_THREAD_T* thread_p)
{
    const char *myname = "thpool_thread_do";
    THPOOL_T *tp_p = thread_p->thpool_p;

    /* Mark thread as alive (initialized) */
    pthread_mutex_lock(&tp_p->thcount_lock);
    tp_p->num_threads_alive += 1;
    pthread_mutex_unlock(&tp_p->thcount_lock);

    while(threads_keepalive)
    {
        thpool_bsem_wait(tp_p->jobqueue->queue_sem); 

        if (threads_keepalive)
        {
            pthread_mutex_lock(&tp_p->thcount_lock);
            tp_p->num_threads_working++;
            pthread_mutex_unlock(&tp_p->thcount_lock);
            /* Read job from queue and execute it */
            void* (*func_buff)(char* arg);
            char* arg_buff;    

            THPOOL_JOB_T *job_p;
            pthread_mutex_lock(&tp_p->jobqueue->rwmutex);                  /* LOCK */

            job_p = thpool_jobqueue_pull(tp_p);

            pthread_mutex_unlock(&tp_p->jobqueue->rwmutex); /* UNLOCK */
            if(job_p) {           
                func_buff=job_p->callback;
                arg_buff =job_p->context;
                func_buff(arg_buff); /* run function */
                myfree((char *)job_p);        /* DEALLOC job */
            }
            pthread_mutex_lock(&tp_p->thcount_lock);
            tp_p->num_threads_working--;
            pthread_mutex_unlock(&tp_p->thcount_lock);
        }
    }
    pthread_mutex_lock(&tp_p->thcount_lock);
    tp_p->num_threads_alive --;
    pthread_mutex_unlock(&tp_p->thcount_lock);
    
    return NULL;
}

/* */
static void thpool_thread_init (THPOOL_T* tp_p, THPOOL_THREAD_T** thread_p, int id)
{
    const char *myname = "thpool_thread_init";
    *thread_p = (struct THPOOL_THREAD_T*)mymalloc(sizeof(struct THPOOL_THREAD_T));
    if (thread_p == NULL){
        msg_panic("%s: Could not allocate memory for thread\n", myname);
    }

    (*thread_p)->thpool_p = tp_p;
    (*thread_p)->id       = id;

    pthread_create(&(*thread_p)->pthread, NULL, (void *)thpool_thread_do, (*thread_p));
    pthread_detach((*thread_p)->pthread);

}
/* Frees a thread  */
static void thpool_thread_destroy (THPOOL_THREAD_T* thread_p){
    myfree((char *)thread_p);
}

/* thpool_init - Initialise thread pool */

THPOOL_T* thpool_init(int threads_num)
{
    const char *myname = "thpool_init";
   
    threads_on_hold   = 0;
    threads_keepalive = 1;

    if (!threads_num || threads_num<1) {
        threads_num=1;
    }

    THPOOL_T *tp_p = NULL;
    tp_p = (THPOOL_T *)mymalloc(sizeof(THPOOL_T));
    if(tp_p == NULL) {
        msg_error("%s: Could not allocate memory for thread pool.", myname);
        return NULL;
    }
    
    tp_p->num_threads_alive   = 0;
    tp_p->num_threads_working = 0;

    /* Initialise the job queue */
    if (thpool_jobqueue_init(tp_p)==-1)
    {
        myfree((char *)tp_p);
        msg_error("%s: thpool_init(): Could not allocate memory for job queue.", myname);
        return NULL;
    }

    /* Make threads in pool */
    tp_p->threads = (struct THPOOL_THREAD_T**)mymalloc(threads_num * sizeof(struct THPOOL_THREAD_T));
    if (tp_p->threads == NULL){
        msg_error("%s: Could not allocate memory for threads\n", myname);
        thpool_jobqueue_destroy(tp_p);
        free(tp_p->jobqueue);
        free(tp_p);
        return NULL;
    }

    pthread_mutex_init(&(tp_p->thcount_lock), NULL);
    /* Make threads in pool */
    int t;
    for (t=0; t<threads_num; t++) {
        msg_info("%s:Created thread %d in pool,threadN:%d.", myname, t, threads_num);
        thpool_thread_init(tp_p, &tp_p->threads[t], t);
    }
    
    /* Wait for threads to initialize */
    while (tp_p->num_threads_alive != threads_num) {}

    return tp_p;
}

/* thpool_add_work - Add work to the thread pool */
int thpool_add_work(THPOOL_T* tp_p, THPOOL_NOTIFY_F callback, char* context)
{
    const char *myname = "thpool_add_work";

    THPOOL_JOB_T *new_job;
    new_job=(THPOOL_JOB_T*)mymalloc(sizeof(THPOOL_JOB_T));  /* MALLOC job */

    if(new_job == NULL)
    {
        msg_error("%s: Could not allocate memory for new job.", myname);
        return -1;
    }
    
    new_job->callback = callback;
    new_job->context = context;

    /* add job to queue */
    pthread_mutex_lock(&tp_p->jobqueue->rwmutex); /* LOCK */
    thpool_jobqueue_push(tp_p, new_job);
    pthread_mutex_unlock(&tp_p->jobqueue->rwmutex); /* UNLOCK */

    return 0;
}
/* Wait until all jobs have finished */
void thpool_wait(THPOOL_T* tp_p)
{
    const char *myname = "thpool_wait";
    /* Continuous polling */
    double timeout = 1.0;
    time_t start, end;
    double tpassed = 0.0;
    time (&start);

    while (tpassed < timeout && 
            (tp_p->jobqueue->len || tp_p->num_threads_working))
    {
        time (&end);
        tpassed = difftime(end,start);
    }

    /* Exponential polling */
    long init_nano =  1; /* MUST be above 0 */
    long new_nano;
    double multiplier =  1.01;
    int  max_secs   = 20;

    struct timespec polling_interval;
    polling_interval.tv_sec  = 0;
    polling_interval.tv_nsec = init_nano;

    while (tp_p->jobqueue->len || tp_p->num_threads_working)
    {
        nanosleep(&polling_interval, NULL);
        if ( polling_interval.tv_sec < max_secs ){
            new_nano = CEIL(polling_interval.tv_nsec * multiplier);
            polling_interval.tv_nsec = new_nano % MAX_NANOSEC;
            if ( new_nano > MAX_NANOSEC ) {
                polling_interval.tv_sec ++;
            }
        }
        else { 
            break;
        }
    }

    /* Fall back to max polling */
    while (tp_p->jobqueue->len || tp_p->num_threads_working){
        sleep(max_secs);
    }
}


/* thpool_destroy - Destroy the threadpool */
void thpool_destroy(THPOOL_T* tp_p)
{
    const char *myname = "thpool_destroy";
    volatile int threads_total = tp_p->num_threads_alive;

    /* End each thread's infinite loop */
    threads_keepalive=0;

    /* Give one second to kill idle threads */
    double TIMEOUT = 1.0;
    time_t start, end;
    double tpassed = 0.0;
    time (&start);
    while (tpassed < TIMEOUT && tp_p->num_threads_alive){
        thpool_bsem_post_all(tp_p->jobqueue->queue_sem);
        time (&end);
        tpassed = difftime(end,start);
    }

    /* Poll remaining threads */
    while (tp_p->num_threads_alive){
        thpool_bsem_post_all(tp_p->jobqueue->queue_sem);
        sleep(1);
    }

    /* Job queue cleanup */
    thpool_jobqueue_destroy(tp_p);
    myfree((char *)tp_p->jobqueue);

    /* Deallocs */
    int n;
    for (n=0; n < threads_total; n++){
        thpool_thread_destroy(tp_p->threads[n]);
    }
    myfree((char *)tp_p->threads);
    myfree((char *)tp_p);
}
