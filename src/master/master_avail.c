/*
 * @Name: master_avail.c
 */



/* System libraries. */


/* Utility library. */

#include "events.h"
#include "msg.h"

/* Application-specific. */

#include "master_proto.h"
#include "master.h"


/* master_avail_event - create child process to handle connection request */

static void master_avail_event(int event, char *context)
{
	MASTER_SERV *serv = (MASTER_SERV *) context;
	int     n;

	if (event == 0)             /* XXX Can this happen? */
		return;
	if (MASTER_THROTTLED(serv)) 
	{       /* XXX interface botch */
		event_disable_rw(master_el, serv->listen_fd, EVENT_READ);
	} 
	else 
	{
		master_spawn(serv);
	}

}

/* master_avail_listen - make sure that someone monitors the listen socket */

void master_avail_listen(MASTER_SERV *serv)
{
	char   *myname = "master_avail_listen";
	int     listen_flag;
	/*
	 * When no-one else is monitoring the service's listen socket, start
	 * monitoring the socket for connection requests. All this under the
	 * restriction that we have sufficient resources to service a connection
	 * request.
	 */

	msg_info("%s: avail %d total %d max %d", myname,
			serv->avail_proc, serv->total_proc, serv->max_proc);

	if (MASTER_THROTTLED(serv) || serv->avail_proc > 0) 
	{
		listen_flag = 0;
	} 
	else if (MASTER_LIMIT_OK(serv->max_proc, serv->total_proc)) 
	{
		listen_flag = 1;
	} 
	else 
	{
		listen_flag = 0;
	}

	if (listen_flag && !MASTER_LISTENING(serv)) 
	{
		msg_info("%s: enable events %s", myname, serv->name);
		event_enable_rw(master_el, serv->listen_fd, master_avail_event,
				(char *) serv, EVENT_READ);
		serv->flags |= MASTER_FLAG_LISTEN;
	} 
	else if (!listen_flag && MASTER_LISTENING(serv)) 
	{
		msg_info("%s: disable events %s", myname, serv->name);
		event_disable_rw(master_el, serv->listen_fd, EVENT_READ);
		serv->flags &= ~MASTER_FLAG_LISTEN;
	}

}

/* master_avail_cleanup - cleanup */

void  master_avail_cleanup(MASTER_SERV *serv)
{

	master_delete_children(serv);       /* XXX calls
										 * master_avail_listen */
	if (MASTER_LISTENING(serv)) 
	{
		event_disable_rw(master_el, serv->listen_fd, EVENT_READ); /* XXX must be last */
		serv->flags &= ~MASTER_FLAG_LISTEN;
	}
}

/* master_avail_more - one more available child process */

void master_avail_more(MASTER_SERV *serv, MASTER_PROC *proc)
{
	char   *myname = "master_avail_more";

	/*
	 * This child process has become available for servicing connection
	 * requests, so we can stop monitoring the service's listen socket. The
	 * child will do it for us.
	 */

	msg_info("%s: pid %d (%s)", myname, proc->pid, proc->serv->name);

	if (proc->avail == MASTER_STAT_AVAIL)
		msg_panic("%s: process already available", myname);

	serv->avail_proc++;
	proc->avail = MASTER_STAT_AVAIL;
//	msg_info("%s: disable events %s", myname, serv->name);
//	event_disable_read(serv->listen_fd); 
	master_avail_listen(serv);
}

/* master_avail_less - one less available child process */

void master_avail_less(MASTER_SERV *serv, MASTER_PROC *proc)
{
	char   *myname = "master_avail_less";

	/*
	 * This child is no longer available for servicing connection requests.
	 * When no child processes are available, start monitoring the service's
	 * listen socket for new connection requests.
	 */
	msg_info("%s: pid %d (%s)", myname, proc->pid, proc->serv->name);
	if (proc->avail != MASTER_STAT_AVAIL)
		msg_panic("%s: process not available", myname);

	serv->avail_proc--;
	proc->avail = MASTER_STAT_TAKEN;
	master_avail_listen(serv);
}
