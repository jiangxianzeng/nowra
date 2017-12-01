/*
 *
 *
 * */


/* System libraries. */
#include <sys/epoll.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stddef.h>

/* Application-specific. */

#include "mymalloc.h"
#include "msg.h"
#include "events.h"
#include "ring.h"
#include "iostuff.h"

static time_t event_present;        /* cached time of day */

#define EVENT_INIT_NEEDED() (event_present == 0)

/*
 * Timer events. Timer requests are kept sorted, in a circular list. We use
 * the RING abstraction, so we get to use a couple ugly macros.
 */

typedef struct EVENT_TIMER {
	time_t  when;           		/* when event is wanted */
	EVENT_NOTIFY_TIME callback;     /* callback function */
	char   *context;            	/* callback context */
	RING    ring;           		/* linkage */
} EVENT_TIMER;

static RING event_timer_head;        /* timer queue head */

#define RING_TO_TIMER(r) \
	((EVENT_TIMER *) ((char *) (r) - offsetof(EVENT_TIMER, ring)))

#define FOREACH_QUEUE_ENTRY(entry, head) \
	for (entry = RING_SUCC(head); entry != (head); entry = RING_SUCC(entry))

#define FIRST_TIMER(head) \
	(RING_SUCC(head) != (head) ? RING_TO_TIMER(RING_SUCC(head)) : 0)


/* event_init - init event  */

event_loop *event_init(int size)
{
    char *myname = "event_init";

    event_loop *evlp;
    if ((evlp = (event_loop *) mymalloc(sizeof(*evlp))) == NULL) goto err;

    if ((evlp->reg_events = (event_base *) mymalloc(sizeof(event_base)*size)) == NULL) goto err;

    evlp->size = size;
    evlp->maxfd = -1;

    if ((evlp->ep_events = (struct epoll_event *) mymalloc(sizeof(struct epoll_event)*size))==NULL) goto err;

    if ((evlp->epfd = epoll_create(EVENT_SIZE)) == -1) goto err;
    int i=0; 
    for (i = 0; i < size; i++) {
        evlp->reg_events[i].mask = EVENT_NONE;
    }
    return evlp;
err:
    if (evlp != NULL) {
        if(evlp->reg_events != NULL){
            myfree((char *)evlp->reg_events);
        }
        if(evlp->ep_events != NULL){
            myfree((char *)evlp->ep_events);
        }
        myfree((char *)evlp);
    }
    return NULL;
}

/* event_free - */
void event_free(event_loop *evlp)
{
    char *myname = "event_free";
    if (evlp != NULL) {
        close(evlp->epfd);
        myfree((char *)evlp->ep_events);
        myfree((char *)evlp->reg_events);
        myfree((char *)evlp);
    }
}

/* event_extend - make room for more descriptor slots */

static void event_extend(int fd)
{
    const char *myname = "event_extend";
}



/* event_drain - loop until all pending events are done */

void event_drain(int time_limit)
{
}

/* event_enable_rw - enable read events or write events */

int event_enable_rw(event_loop *evlp, int fd, EVENT_NOTIFY_RDWR callback, char *context, int mask)
{
    const char *myname = "event_enable_rw";
    /*
     * Sanity checks.
     */
    if (evlp == NULL) {
        msg_panic("%s: event no init.", myname);
    }
    if (fd < 0)
        msg_panic("%s: bad file descriptor: %d", myname, fd);
    
    if (fd >= evlp->size) {
        errno = ERANGE;
        msg_warn("%s: file descriptor is more than event loop size.fd: %d", myname, fd);
        return -1;
    }

    if (msg_verbose > 2) {
        msg_info("%s: fd %d", myname, fd);
    }

    event_base *eb = &evlp->reg_events[fd];
    
    struct epoll_event ee;
    int op = eb->mask == EVENT_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    ee.events = 0;
    eb->mask |= mask;
    if (eb->mask & EVENT_READ) {
        ee.events |= EPOLLIN;
    }
    if (eb->mask & EVENT_WRITE) {
        ee.events |= EPOLLOUT;
    }

    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (epoll_ctl(evlp->epfd, op, fd, &ee) == -1) {
        msg_error("%s: epool ctl failed.error:%s.",myname, strerror(errno));
        return -1;
    }
    if (mask & EVENT_READ) {
        eb->read_cb = callback;
    }
    if (mask & EVENT_WRITE) {
        eb->write_cb = callback;
    }

    eb->client_data = context;
    if (fd > evlp->maxfd) {
        evlp->maxfd = fd;
    }

    return 0;
}

/* event_disable_rw - disable request for read events or wrtie events */

int event_disable_rw(event_loop *evlp, int fd, int mask)
{
    const char *myname = "event_disable_rw";
    /*
     * Sanity checks.
     */
    if (evlp == NULL) {
        msg_panic("%s: event no init.", myname);
    }
    if (fd < 0)
        msg_panic("%s: bad file descriptor: %d", myname, fd);
    
    if (fd >= evlp->size) {
        errno = ERANGE;
        msg_warn("%s: file descriptor is more than event loop size.fd: %d", myname, fd);
        return -1;
    }

    event_base *eb = &evlp->reg_events[fd];
    if (eb->mask == EVENT_NONE) return 0;

    struct epoll_event ee;
    int delmask = eb->mask & (~mask);

    ee.events = 0;
    if (delmask & EVENT_READ) ee.events |= EPOLLIN;
    if (delmask & EVENT_WRITE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; 
    ee.data.fd = fd;
    if (delmask != EVENT_NONE) {
        if (epoll_ctl(evlp->epfd,EPOLL_CTL_MOD,fd,&ee) == -1) {
            msg_error("%s: epoll ctl mod failed. error:%s.", myname, strerror(errno));
            return -1;
        }
    } else {
        if (epoll_ctl(evlp->epfd,EPOLL_CTL_DEL,fd,&ee) == -1) {
            msg_error("%s: epoll ctl  del failed. error:%s.", myname, strerror(errno));
            return -1;
        }
    }
    
    eb->mask = eb->mask & (~mask);
    if (fd == evlp->maxfd && eb->mask == EVENT_NONE) {
        /* Update the max fd */
        int j;
        for (j = evlp->maxfd-1; j >= 0; j--)
            if (evlp->reg_events[j].mask != EVENT_NONE) break;
        evlp->maxfd = j;
    }
    return 0;
}

/* event_time - look up cached time of day */

static void  event_time_init(void)
{
	/*
	 * Initialize timer stuff.
	 */
	ring_init(&event_timer_head);

    (void) time(&event_present);
}

/* event_request_timer - (re)set timer */

time_t event_request_timer(EVENT_NOTIFY_TIME callback, char *context, int delay)
{
	char   *myname = "event_request_timer";
	RING   *ring;
	EVENT_TIMER *timer;

	if (event_present == 0) {
        event_time_init();
    }

	/*
	 * Sanity checks.
	 */
	if (delay < 0)
		msg_panic("%s: invalid delay: %d", myname, delay);

	/*
	 * Make sure we schedule this event at the right time.
	 */
	time(&event_present);

	/*
	 * See if they are resetting an existing timer request. If so, take the
	 * request away from the timer queue so that it can be inserted at the
	 * right place.
	 */

	FOREACH_QUEUE_ENTRY(ring, &event_timer_head)
	{
		timer = RING_TO_TIMER(ring);
		if (timer->callback == callback && timer->context == context) 
		{
			timer->when = event_present + delay;
			ring_detach(ring);
			msg_info("%s: reset 0x%lx 0x%lx %d", myname,
					(long) callback, (long) context, delay);
			break;
		}
	}

	/*
	 * If not found, schedule a new timer request.
	 */
	if (ring == &event_timer_head) 
	{
		timer = (EVENT_TIMER *) mymalloc(sizeof(EVENT_TIMER));
		timer->when = event_present + delay;
		timer->callback = callback;
		timer->context = context;
		msg_info("%s: set 0x%lx 0x%lx %d", myname,
				(long) callback, (long) context, delay);
	}


	/*
	 * Insert the request at the right place. Timer requests are kept sorted
	 * to reduce lookup overhead in the event loop.
	 */
	FOREACH_QUEUE_ENTRY(ring, &event_timer_head)
	{
		if (timer->when < RING_TO_TIMER(ring)->when)
			break;
	}
	ring_prepend(ring, &timer->ring);

	return (timer->when);
}

/* event_cancel_timer - cancel timer */

int event_cancel_timer(EVENT_NOTIFY_TIME callback, char *context)
{
	char   *myname = "event_cancel_timer";
	RING   *ring;
	EVENT_TIMER *timer;
	int time_left = -1;

	if (event_present == 0) {
        event_time_init();
    }

	/*
	 * See if they are canceling an existing timer request. Do not complain
	 * when the request is not found. It might have been canceled from some
	 * other thread.
	 */
	FOREACH_QUEUE_ENTRY(ring, &event_timer_head) 
	{
		timer = RING_TO_TIMER(ring);
		if (timer->callback == callback && timer->context == context) 
		{
			if ((time_left = timer->when - event_present) < 0)
				time_left = 0;
			ring_detach(ring);
			myfree((char *) timer);
			break;
		}
	}
	msg_info("%s: 0x%lx 0x%lx %d", myname,
			(long) callback, (long) context, time_left);
	return (time_left);
}
/* event_loop - wait for the next event */

void event_wait(event_loop *evlp, int delay)
{
    const char *myname = "event_wait";
    static int nested = -1;

    int event_count;
	EVENT_TIMER *timer;
    int fd;
    int select_delay = delay;

    if (evlp == NULL) {
        msg_panic("%s: event no init.", myname);
    }

	if (event_present == 0) {
        event_time_init();
    }

	/*
	 * Find out when the next timer would go off. Timer requests are sorted.
	 * If any timer is scheduled, adjust the delay appropriately.
	 */
	if ((timer = FIRST_TIMER(&event_timer_head)) != 0) {
		event_present = time((time_t *) 0);
		if ((select_delay = timer->when - event_present) < 0) {
			select_delay = 0;
		} else if (delay >= 0 && select_delay > delay) {
			select_delay = delay;
		}
	} else {
		select_delay = delay;
	}
	
	msg_debug("event_loop: select_delay %d", select_delay);

    event_count = epoll_wait(evlp->epfd, evlp->ep_events, evlp->size,(select_delay) < 0 ? -1 : (select_delay) * 1000 );
    if (event_count < 0) {
        if (errno != EINTR)
            msg_fatal("event_loop: %s: %s", "epoll_wait", strerror(errno));
        return;
    }

    if (nested++ > 0)
        msg_panic("event_loop: recursive call");

	event_present = time((time_t *) 0);

	while ((timer = FIRST_TIMER(&event_timer_head)) != 0) {
		if (timer->when > event_present)
			break;
		ring_detach(&timer->ring);		/* first this */
		msg_debug("%s: timer 0x%lx 0x%lx", myname,
					(long) timer->callback, (long) timer->context);
		timer->callback(EVENT_TIME, timer->context);	/* then this */
		myfree((char *) timer);
	}
    int j=0;
    for (j = 0; j < event_count; j++) {   
        struct epoll_event *e = evlp->ep_events+j;
        fd = e->data.fd;
        if (fd < 0 || fd > evlp->maxfd) {
            msg_panic("%s: bad file descriptor: %d", myname, fd);
        }
        event_base *eb = &evlp->reg_events[fd];
        if (e->events & EPOLLIN) {
            if (msg_verbose > 2)
                msg_info("%s: read fd=%d act=0x%lx 0x%lx", myname,
                        fd, (long) eb->read_cb, (long) eb->client_data);
           eb->read_cb(fd, eb->client_data); 
        } else if (e->events & EPOLLOUT) {
            if (msg_verbose > 2)
                msg_info("%s: write fd=%d act=0x%lx 0x%lx", myname,
                        fd, (long) eb->write_cb,
                        (long) eb->client_data);
            eb->write_cb(fd, eb->client_data);
        } else {
            msg_error("%s: epoll wait some error.",myname);
        }
    }
   nested--; 
}


#ifdef TEST
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

/* timer_event - display event */

static void timer_event(int unused_event, char *context)
{
	printf("%ld: %s\n", (long) event_present, context);
	fflush(stdout);
}

/* echo - echo text received on stdin */

static void echo(int unused_event, char *unused_context)
{
	char    buf[BUFSIZ];

	if (fgets(buf, sizeof(buf), stdin) == 0)
		exit(0);
	printf("Result: %s", buf);
}

/* request - request a bunch of timer events */

static void request(int unused_event, char *unused_context)
{
	event_request_timer(timer_event, "3 first", 3);
	event_request_timer(timer_event, "3 second", 3);
	event_request_timer(timer_event, "4 first", 4);
	event_request_timer(timer_event, "4 second", 4);
	event_request_timer(timer_event, "2 first", 2);
	event_request_timer(timer_event, "2 second", 2);
	event_request_timer(timer_event, "1 first", 1);
	event_request_timer(timer_event, "1 second", 1);
	event_request_timer(timer_event, "0 first", 0);
	event_request_timer(timer_event, "0 second", 0);
}

int main(int argc, char **argv)
{
	event_request_timer(request, (char *) 0, 0);
//	event_enable_read(fileno(stdin), echo, (char *) 0);
	while(1)
	{
		event_loop(1);
	};
	msg_info("TEST+++++++++++++");
	exit(0);
}

#endif
