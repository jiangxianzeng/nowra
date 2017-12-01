/*
 * @Name: master.c
 */


/* System libraries. */

#include <sys/stat.h>

/* Utility library. */

#include "msg.h"
/* Application-specific. */

#include "master.h"

/* main - main program */

event_loop *master_el;

int main(int argc, char **argv)
{
	const char *myname = "main";
//	msg_filelog_init(0, "/tmp/test.log");

	int i=0;
	for(i=0;i<5;i++)
	{
		dup(0);
	}

    master_el = event_init(MASTER_MAX_CLIENTS);

	fset_master_ent("master.cf");
	master_config();
	msg_info("daemon started");

	for (;;) 
	{
		event_wait(master_el, 2);
	}
	//	msg_filelog_free();
	return 0;
}
