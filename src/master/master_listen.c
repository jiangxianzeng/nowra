/*
 * @Name:master_listen.c
 *
 */


/* System library. */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "stringop.h"
#include "iostuff.h"

/* Application-specific. */

#include "master.h"

/* master_listen_init - enable connection requests */
void master_listen_init(MASTER_SERV *serv)
{
	char   *myname = "master_listen_init";
	char   *end_point;
	int     n;

	switch (serv->type) 
	{
		/*
		 * UNIX-domain or stream listener endpoints always come as singlets.
		 */
		case MASTER_SERV_TYPE_UNIX:
			break;

		/*
		 * FIFO listener endpoints always come as singlets.
		 */
		case MASTER_SERV_TYPE_FIFO:
			break;
		/*
		 * INET-domain listener endpoints can be wildcarded (the default) or
		 * bound to specific interface addresses.
		 */
		case MASTER_SERV_TYPE_INET:
			{
				if(serv->host == NULL)
				{
					end_point = concatenate("",":",serv->port, (char *)0);
				}
				else
				{
					end_point = concatenate(serv->host,":",serv->port, (char *)0);
				}
				serv->listen_fd = inet_listen(end_point, serv->max_proc > var_proc_limit ? \
												serv->max_proc : var_proc_limit, NON_BLOCKING);
				msg_debug("%s: listen_fd:%d, name:%s.", myname, serv->listen_fd, serv->name);
				close_on_exec(serv->listen_fd, CLOSE_ON_EXEC); 
				myfree(end_point);
			}
			break;

		default:
			msg_panic("%s: unknown service type: %d", myname, serv->type);
	}

}

/* master_listen_cleanup - disable connection requests */

void master_listen_cleanup(MASTER_SERV *serv)
{

	char   *myname = "master_listen_cleanup";
	int     n;

	if (close(serv->listen_fd) < 0)
		msg_warn("%s: close listener socket %d: %s",
				myname, serv->listen_fd, strerror(errno));
	serv->listen_fd = -1;
}
