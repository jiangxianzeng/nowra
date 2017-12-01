/*
 * @Name: master_proto.c
 *
 */

/* System library. */

#include <unistd.h>

/* Utility library. */

#include "msg.h"

/* Global library. */

#include "master_proto.h"

int master_notify(int pid, int status)
{
    char   *myname = "master_notify";
    MASTER_STATUS stat;

     /*
     * We use a simple binary protocol to minimize security risks. Since this
     * is local IPC, there are no byte order or word length issues. The
     * server treats this information as gossip, so sending a bad PID or a
     * bad status code will only have amusement value.
     */
    stat.pid = pid;
    stat.avail = status;

	if (write(MASTER_STATUS_FD, (char *) &stat, sizeof(stat)) != sizeof(stat)) 
	{
		msg_info("%s: status %d: %m", myname, status);
		return (-1);
	} 
	else 
	{
		msg_info("%s: status %d", myname, status);
		return (0);
	}
}
