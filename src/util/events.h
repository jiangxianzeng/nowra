#ifndef _EVENTS_H_INCLUDED_
#define _EVENTS_H_INCLUDED_

#include <time.h>

 /*
  *   * External interface.
  *     */

#define EVENT_SIZE  1024

typedef void (*EVENT_NOTIFY_RDWR) (int fd, char *context);
typedef void (*EVENT_NOTIFY_TIME) (int fd, char *context);

typedef struct event_base {
    int mask;
    EVENT_NOTIFY_RDWR read_cb;
    EVENT_NOTIFY_RDWR write_cb;
    void *client_data;
} event_base;

typedef struct event_loop {
    int maxfd;   /* highest file descriptor currently registered */
    int size; /* max number of file descriptors tracked */

    event_base *reg_events; /* Registered events */

    int epfd;      /* epoll descriptor */
    struct epoll_event *ep_events;  /* event[] - events that were triggered */
} event_loop;


extern event_loop *event_init(int size);
extern void event_free(event_loop *evlp);

extern int event_enable_rw(event_loop *evlp, int fd, EVENT_NOTIFY_RDWR callback, char *context, int mask);
extern int event_disable_rw(event_loop *evlp, int fd, int mask);


//extern time_t event_time(void);
extern time_t event_request_timer(EVENT_NOTIFY_TIME callback, char *context, int delay);
extern int event_cancel_timer(EVENT_NOTIFY_TIME callback, char *context);

extern void event_wait(event_loop *evlp, int delay);
extern void event_drain(int time_limit);

 /*
  * Event codes.
  */
#define EVENT_NONE  (0<<0)      /* */
#define EVENT_READ  (1<<0)      /* read event */
#define EVENT_WRITE (1<<1)      /* write event */
#define EVENT_XCPT  (1<<2)      /* exception */
#define EVENT_TIME  (1<<3)      /* timer event */

#define EVENT_ERROR EVENT_XCPT

 /*
  * Dummies.
  */
#define EVENT_NULL_TYPE 0
#define EVENT_NULL_CONTEXT  ((char *) 0)


#endif
