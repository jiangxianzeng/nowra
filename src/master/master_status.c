/*
 * @Name: master_status.c
 *
 */


/* System libraries. */

#include <unistd.h>

/* Utility library. */

#include "msg.h"
#include "events.h"
#include "binhash.h"
#include "iostuff.h"

/* Application-specific. */

#include "master_proto.h"
#include "master.h"

/* master_status_event - status read event handler */

static void master_status_event(int event, char *context)
{
	char   *myname = "master_status_event";
	MASTER_SERV *serv = (MASTER_SERV *) context;
	MASTER_STATUS stat;
	MASTER_PROC *proc;
	MASTER_PID pid;
	int     n;

	if (event == 0)             /* XXX Can this happen?  */
		return;

	/*
	 * We always keep the child end of the status pipe open, so an EOF read
	 * condition means that we're seriously confused. We use non-blocking
	 * reads so that we don't get stuck when someone sends a partial message.
	 * Messages are short, so a partial read means someone wrote less than a
	 * whole status message. Hopefully the next read will be in sync again...
	 * We use a global child process status table because when a child dies
	 * only its pid is known - we do not know what service it came from.
	 */
	switch (n = read(serv->status_fd[0], (char *) &stat, sizeof(stat)))
	{
		case -1:
			msg_warn("%s: read: %m", myname);
			return;
		case 0:
			msg_panic("%s: read EOF status", myname);
			/* NOTREACHED */
		default:
			msg_warn("%s: partial status (%d bytes)", myname, n);
			return;
		case sizeof(stat):
			pid = stat.pid;
			msg_info("%s: pid %d avail %d", myname, stat.pid, stat.avail);
	}
	/*
	 * Sanity checks. Do not freak out when the child sends garbage because
	 * it is confused or for other reasons. However, be sure to freak out
	 * when our own data structures are inconsistent. A process not found
	 * condition can happen when we reap a process before receiving its
	 * status update, so this is not an error.
	 */

	if ((proc = (MASTER_PROC *) binhash_find(master_child_table, (char *) &pid, sizeof(pid))) == 0) 
	{
		msg_info("%s: process id not found: %d", myname, stat.pid);
		return;
	}

	if (proc->serv != serv)
		msg_panic("%s: pointer corruption: %p != %p", myname, (void *) proc->serv, (void *) serv);

	/*
	 * Update our idea of the child process status. Allow redundant status
	 * updates, because different types of events may be processed out of
	 * order. Otherwise, warn about weird status updates but do not take
	 * action. It's all gossip after all.
	 */
	if (proc->avail == stat.avail)
		return;

	switch (stat.avail) 
	{
		case MASTER_STAT_AVAIL:
			proc->use_count++;
			master_avail_more(serv, proc);
			break;
		case MASTER_STAT_TAKEN:
			master_avail_less(serv, proc);
			break;
		default:
			msg_warn("%s: ignoring unknown status: %d allegedly from pid: %d", myname, stat.pid, stat.avail);
			break;
	}
}

/* master_status_init - start status event processing for this service */

void master_status_init(MASTER_SERV *serv)
{
	char   *myname = "master_status_init";

	/*
	 * Sanity checks.
	 */

	if (serv->status_fd[0] >= 0 || serv->status_fd[1] >= 0)
		msg_panic("%s: status events already enabled", myname);
	msg_info("%s: %s", myname, serv->name);

	/*
	 * Make the read end of this service's status pipe non-blocking so that
	 * we can detect partial writes on the child side. We use a duplex pipe
	 * so that the child side becomes readable when the master goes away.
	 */

	if (pipe(serv->status_fd) < 0)
		msg_fatal("pipe: %m");

	non_blocking(serv->status_fd[0], BLOCKING);
	close_on_exec(serv->status_fd[0], CLOSE_ON_EXEC);
	close_on_exec(serv->status_fd[1], CLOSE_ON_EXEC);
	event_enable_rw(master_el, serv->status_fd[0], master_status_event, (char *) serv, EVENT_READ);
}

/* master_status_cleanup - stop status event processing for this service */

void master_status_cleanup(MASTER_SERV *serv)
{
	char *myname = "master_status_cleanup";

	/*
	 * Sanity checks.
	 */
	if (serv->status_fd[0] < 0 || serv->status_fd[1] < 0)
		msg_panic("%s: status events not enabled", myname);
	msg_info("%s: %s", myname, serv->name);

	/*
	 * Dispose of this service's status pipe after disabling read events.
	 */

	event_disable_rw(master_el, serv->status_fd[0], EVENT_READ);
	if (close(serv->status_fd[0]) != 0)
		msg_warn("%s: close status descriptor (read side): %m", myname);
	if (close(serv->status_fd[1]) != 0)
		msg_warn("%s: close status descriptor (write side): %m", myname);
	serv->status_fd[0] = serv->status_fd[1] = -1;
}
