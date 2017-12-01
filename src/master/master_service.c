/*
 * @Name: master_service.c
 *
 */

/* System libraries. */

#include <string.h>
#include <unistd.h>


/* Utility library. */

#include "msg.h"
#include "mymalloc.h"

/* Application-specific. */

#include "master.h"

MASTER_SERV *master_head;

/* master_start_service - activate service */

void master_start_service(MASTER_SERV *serv)
{
	/*
	 * Enable connection requests, wakeup timers, and status updates from
	 * child processes.
	 */
	master_listen_init(serv);
	
	master_avail_listen(serv);

	master_status_init(serv);

}

/* master_stop_service - deactivate service */

void master_stop_service(MASTER_SERV *serv)
{
	/*
	 * Undo the things that master_start_service() did.
	 */

	master_status_cleanup(serv);
	master_avail_cleanup(serv);
	master_listen_cleanup(serv);
}

/* master_restart_service - restart service after configuration reload */

void master_restart_service(MASTER_SERV *serv)
{
	/*
	 * Undo some of the things that master_start_service() did.
	 */
	master_status_cleanup(serv);

	/*
	 * Now undo the undone.
	 */
	master_status_init(serv);
}
