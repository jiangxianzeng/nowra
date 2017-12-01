#ifndef _THPOOL_H_INCLUDE_
#define _THPOOL_H_INCLUDE_

 /*
  * System library.
  */

/*
 * Utility library.
 * */

 
typedef struct THPOOL_T THPOOL_T;

typedef void *(*THPOOL_NOTIFY_F) (char *);

/**
 *  @brief  Initialize threadpool
 *   
 *  Initializes a threadpool. This function will not return untill all
 *  threads have initialized successfully. 
 *   
 *  @param  threadsN number of threads to be created in the threadpool
 */
THPOOL_T* thpool_init(int threads_num);

/*
 *  @brief Add work to the job queue
 *
 * */
int thpool_add_work(THPOOL_T* tp_p, THPOOL_NOTIFY_F callback, char* context);


/**
 * @brief Wait for all queued jobs to finish
 *
 * */
void thpool_wait(THPOOL_T* tp_p);

/*
 * @brief Destroy the threadpool
 * */
void thpool_destroy(THPOOL_T* tp_p);

#endif
