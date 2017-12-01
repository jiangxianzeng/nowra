#include <stdio.h>
#include "thpool.h"
#include "msg.h"
#include "msg_filelog.h"
#include "stringop.h"
#include "vstring.h"

/* Some arbitrary task 1 */
static int count = 0;
void task1()
{
    printf("# Thread working: %X\n", (int)pthread_self());
    printf(" Task 1 running..%d.\n",++count);
}

/* Some arbitrary task 2 */
void task2(int a){
    printf("# Thread working: %u\n", (int)pthread_self());
    printf(" Task 2 running..\n");
    printf("%d\n", a);
}

int main()
{
    char *myname = "main";
    int i;
    msg_filelog_init(2, "/tmp/test.log");

    msg_info("%s: file log test.", myname);
/*    VSTRING *vp = vstring_alloc(1);
    printf("vp str len:%d\n", VSTRING_LEN(vp));
    vp = vstring_strcpy(vp, "jiang");
    vstring_range(vp, 6, -1);
    printf("vp: %s\n", VSTRING_STR(vp));
    printf("vp len: %d\n", VSTRING_LEN(vp));
    printf("vp avail: %d\n", VSTRING_AVAIL(vp));

    vstring_free(vp); */

    THPOOL_T* threadpool; /* make a new thread pool structure */
    threadpool=thpool_init(16); /* initialise it to 4 number of threads */


    puts("Adding 10 tasks to threadpool");
    int a=54;
    for (i=0; i<20000; i++) {
        thpool_add_work(threadpool, (void*)task1, NULL);
        thpool_add_work(threadpool, (void*)task2, (void*)a);
    }

    thpool_wait(threadpool);
    puts("Will kill threadpool");
    thpool_destroy(threadpool);

    printf("+++++++++++++++++end+++++++++++\n");
    msg_filelog_free();
    return 0;
}
