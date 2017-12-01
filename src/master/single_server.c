/*
 * @Name: single_server.c
 *
 */

/* System library. */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

/* Utility library. */
#include "msg.h"
#include "vstring.h"
#include "mymalloc.h"
#include "events.h"
#include "iostuff.h"
#include "inet_stuff.h"
#include "stringop.h"

/* Process manager. */

#include "master_proto.h"

/* Application-specific */

#include "app_server.h"

static int var_pid;

static int use_count;

static void (*single_server_service) (int, char *, char **);
static char *single_server_name;
static char **single_server_argv;
static void (*single_server_accept) (int, char *);
static void (*single_server_onexit) (char *, char **);
static void (*single_server_pre_accept) (char *, char **);

/* single_server_wakeup - wake up application */

static void single_server_wakeup(int fd)
{
	const char *myname = "single_server_wakeup";
	/*
	 * If the accept() succeeds, be sure to disable non-blocking I/O, because
	 * the application is supposed to be single-threaded. Notice the master
	 * of our (un)availability to service connection requests. Commit suicide
	 * when the master process disconnected from us.
	 */
	msg_info("connection established");
	non_blocking(fd, BLOCKING);
	close_on_exec(fd, CLOSE_ON_EXEC);

	if (master_notify(var_pid, MASTER_STAT_TAKEN) < 0)
	{
		msg_warn("%s: master_notify failed. MASTER_STAT_TAKEN", myname);
		//single_server_abort(EVENT_NULL_TYPE, EVENT_NULL_CONTEXT);
	}

	single_server_service(fd, single_server_name, single_server_argv);
	
	close(fd);

	if (master_notify(var_pid, MASTER_STAT_AVAIL) < 0)
	{

		msg_warn("%s: master_notify failed.MASTER_STAT_AVAIL",myname);
		//single_server_abort(EVENT_NULL_TYPE, EVENT_NULL_CONTEXT);
	}
	if (msg_verbose)
		msg_info("connection closed");
	use_count++;
}

/* single_server_accept_inet - accept client connection request */

static void single_server_accept_inet(int unused_event, char *context)
{
	const char *myname = "single_server_accept_inet";
	int     listen_fd = (int) (long)(context);
	int     time_left = -1;
	int     fd;

	msg_info("single_server_accept_inet line: %d.", __LINE__);
	if (single_server_pre_accept)
		single_server_pre_accept(single_server_name, single_server_argv);
	fd = inet_accept(listen_fd);

	if (fd < 0) 
	{
		if (errno != EAGAIN)
		{
			msg_fatal("%s: accept connection: %s.", myname, strerror(errno));
		}
		else
		{
			msg_debug("%s: accept will try again.", myname);
		}
		return;
	}

	single_server_wakeup(fd);
}

/* single_server_main - the real main program */

void single_server_main(int argc, char **argv, SINGLE_SERVER_FN service,...)
{
	char   *myname = "single_server_main";
//	VSTREAM *stream = 0;
	int     delay;
	int     c;
	int     socket_count = 1;
	int     fd;

	/*
	 * Don't die when a process goes away unexpectedly.
	 */
	signal(SIGPIPE, SIG_IGN);

	char *service_name = basename(argv[0]);

	char   *transport = 0;

	var_pid = getpid();

	while ((c = getopt(argc, argv, "cDi:lm:n:o:s:St:uv")) > 0) 
	{
		switch (c) 
		{
			case 'n':
				service_name = optarg;
				break;
			case 't':
				transport = optarg;
				break;
		}
	}

	if (transport == 0)
	{
		msg_fatal("no transport type specified");
	}
	if (strcasecmp(transport, MASTER_XPORT_NAME_INET) == 0)
	{
		single_server_accept = single_server_accept_inet;
	}
	else
	{
		 msg_fatal("unsupported transport type: %s", transport);
	}

	/*
	 * Set up call-back info.
	 */
	single_server_service = service;
	single_server_name = service_name;
	single_server_argv = argv + optind;

	fd = MASTER_LISTEN_FD;
	event_loop *el;
    el = event_init(100);
    if (el == NULL) {
        msg_error("event_init failed.");
    }
	//event_enable_read(fd, single_server_accept, (char *) (long)(fd));
	event_enable_rw(el, fd, single_server_accept, (char *) (long)(fd), EVENT_READ);
	close_on_exec(fd, CLOSE_ON_EXEC);

	while(1) {
		event_wait(el, -1);
	}
}
