/*
 * @Name: master_ent.c
 */


/* System libraries. */

#include <netinet/in.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <strings.h>
#include <stdio.h>

/* Utility libraries. */
#include "mymalloc.h"
#include "vstring.h"
#include "stringop.h"
#include "argv.h"

#include "master.h"
#include "master_proto.h"


int var_proc_limit = 1;
int var_throttle_time = 10;

char *var_daemon_dir = "./";

static char *master_path;       /* config file name */
static FILE *master_fp;			/* config file pointer */
static char master_blanks[] = " \t\r\n";/* field delimiters */

static int master_line;         /* config file line number */


/* fset_master_ent - specify configuration file pathname */

void fset_master_ent(char *path)
{
	if (master_path != 0)
	{
		myfree(master_path);
	}
	master_path = mystrdup(path);
}

/* set_master_ent - open configuration file */

void set_master_ent()
{
	char   *myname = "set_master_ent";

	if (master_fp != 0)
		msg_panic("%s: configuration file still open", myname);
	if (master_path == 0)
		msg_panic("%s: no configuration file specified", myname);
	if ((master_fp = fopen(master_path, "r")) == 0)
		msg_fatal("open %s: %m", master_path);
	master_line = 0;
}

/* end_master_ent - close configuration file */

void end_master_ent()
{
	char *myname = "end_master_ent";

	if (master_fp == 0)
		msg_panic("%s: configuration file not open", myname);
	if (fclose(master_fp) != 0)
		msg_fatal("%s: close configuration file: %m", myname);
	master_fp = 0;
}

/* fatal_with_context - print fatal error with file/line context */

static void fatal_with_context(char *format,...)
{
	char *myname = "fatal_with_context";
	VSTRING *vp = vstring_alloc(100);
	va_list ap;

	if (master_path == 0)
		msg_panic("%s: no configuration file specified", myname);

	va_start(ap, format);
	vstring_vsprintf(vp, format, ap);
	va_end(ap);
	msg_fatal("%s: line %d: %s", master_path, master_line, VSTRING_STR(vp));
}

/* fatal_invalid_field - report invalid field value */

static void fatal_invalid_field(char *name, char *value)
{
	fatal_with_context("field \"%s\": bad value: \"%s\"", name, value);
}

/* get_str_ent - extract string field */

static char *get_str_ent(char **bufp, char *name, char *def_val)
{
	char   *value;

	if ((value = mystrtok(bufp, master_blanks)) == 0)
	{
		fatal_with_context("missing \"%s\" field", name);
	}
	if (strcmp(value, "-") == 0) 
	{
		if (def_val == 0)
			fatal_with_context("field \"%s\" has no default value", name);
		return def_val;
	}
	else
	{
		return (value);
	}
}

/* get_bool_ent - extract boolean field */

static int get_bool_ent(char **bufp, char *name, char *def_val)
{
	char *value;
	value = get_str_ent(bufp, name, def_val);
	if (strcmp("y", value) == 0) 
	{
		return (1);
	} 
	else if (strcmp("n", value) == 0) 
	{
		return (0);
	} 
	else 
	{
		fatal_invalid_field(name, value);
	}
	/* NOTREACHED */
}

/* get_int_ent - extract integer field */

static int get_int_ent(char **bufp, char *name, char *def_val, int min_val)
{
	char   *value;
	int     n;

	value = get_str_ent(bufp, name, def_val);
	if ((n = atoi(value)) < min_val)
		fatal_invalid_field(name, value);
	return (n);
}


/* get_master_ent - read entry from configuration file */

MASTER_SERV *get_master_ent()
{
	const char *myname = "get_master_ent";
	VSTRING *junk = vstring_alloc(100);
	char buf[1024] = {0};
	char *cp;
	char *name;
	char *transport;
	char *host;
	char *port;
	char *command;
	char   *bufp;
	char   *atmp;
	MASTER_SERV *serv;

	if (master_fp == 0)
	{
		msg_panic("get_master_ent: config file not open");
	}

	/*
	 * Skip blank lines and comment lines.
	 */

	do 
	{
		//if (readlline(buf, master_fp, &master_line, READLL_STRIPNL) == 0) 
		if (fgets(buf, 1024, master_fp) == NULL) 
		{
			vstring_free(junk);
			return (0);
		}
		bufp = buf;
		//bufp = vstring_str(buf);
	}while((cp = mystrtok(&bufp, master_blanks)) == 0 || *cp == '#');

	/*
	 * Parse one logical line from the configuration file. Initialize service
	 * structure members in order.
	*/
    serv = (MASTER_SERV *) mymalloc(sizeof(MASTER_SERV));
    serv->next = 0;

    /*
	 * Flags member.
	 */
    serv->flags = 0;	
	
	/*
	 * Service name. Syntax is transport-specific.
	 */
	name = cp;

	/*
	 *      * Transport type: inet (wild-card listen or virtual) or unix.
	 *           */
	transport = get_str_ent(&bufp, "transport type", (char *) 0);
	
	if (!strcmp(transport, MASTER_XPORT_NAME_INET)) 
	{
		serv->type = MASTER_SERV_TYPE_INET;
		atmp = host_port(name, &host, &port);	
		if (*host) 
		{
			serv->flags |= MASTER_FLAG_INETHOST;/* host:port */

			serv->host = mystrdup(host);	
		}
		else
		{
			serv->host = NULL;
		}
		serv->port = mystrdup(port);
	}
	else
	{
		fatal_with_context("bad transport type: %s", transport);
	}

	serv->name = mystrdup(name);

	serv->listen_fd = -1;

	serv->wakeup_time = get_int_ent(&bufp, "wakeup_time", "0", 0);

	/*
	 * Find out if the wakeup time is conditional, i.e., wakeup triggers
	 * should not be sent until the service has actually been used.
	 */
	if (serv->wakeup_time > 0 && buf[*buf ? -2 : -1] == '?')
		serv->flags |= MASTER_FLAG_CONDWAKE;

	/*
	 * Concurrency limit. Zero means no limit.
	 */
	vstring_sprintf(junk, "%d", var_proc_limit);
	serv->max_proc = get_int_ent(&bufp, "max_proc", VSTRING_STR(junk), 0);

	/*
	 * Path to command,
	 */
	command = get_str_ent(&bufp, "command", (char *) 0);
	serv->path = concatenate(var_daemon_dir, "/", command, (char *) 0);

	/*
	 * Idle and total process count.
	 */
	serv->avail_proc = 0;
	serv->total_proc = 0;

	/*
	 * Backoff time in case a service is broken.
	 */
	serv->throttle_delay = var_throttle_time;

	/*
	 * Shared channel for child status updates.
	 */
	serv->status_fd[0] = serv->status_fd[1] = -1;

	/*
	 * Child process structures.
	 */
	serv->children = 0;

	/*
	 * Command-line vector. Add "-n service_name" when the process name
	 * basename differs from the service name. Always add the transport.
	 */
	serv->args = argv_alloc(0);
	argv_add(serv->args, command, (char *) 0);
	if (serv->max_proc == 1)
		argv_add(serv->args, "-l", (char *) 0);
	if (strcmp(basename(command), name) != 0)
		argv_add(serv->args, "-n", name, (char *) 0);
	argv_add(serv->args, "-t", transport, (char *) 0);
	while ((cp = mystrtok(&bufp, master_blanks)) != 0)
		argv_add(serv->args, cp, (char *) 0);
	argv_terminate(serv->args);

	/* Clean up*/
	vstring_free(junk);
	return (serv);
}

/* print_master_ent - show service entry contents */

void print_master_ent(MASTER_SERV *serv)
{
	char  **cpp;

	msg_info("====start service entry");
	msg_info("flags: %d", serv->flags);
	msg_info("name: %s", serv->name);
	msg_info("type: %s",
			serv->type == MASTER_SERV_TYPE_UNIX ? MASTER_XPORT_NAME_UNIX :
			serv->type == MASTER_SERV_TYPE_FIFO ? MASTER_XPORT_NAME_FIFO :
			serv->type == MASTER_SERV_TYPE_INET ? MASTER_XPORT_NAME_INET :
			"unknown transport type");
	//msg_info("listen_fd_count: %d", serv->listen_fd_count);
	msg_info("wakeup: %d", serv->wakeup_time);
	msg_info("max_proc: %d", serv->max_proc);
	msg_info("path: %s", serv->path);
	for (cpp = serv->args->argv; *cpp; cpp++)
		msg_info("arg[%d]: %s", (int) (cpp - serv->args->argv), *cpp);
	msg_info("avail_proc: %d", serv->avail_proc);
	msg_info("total_proc: %d", serv->total_proc);
	msg_info("throttle_delay: %d", serv->throttle_delay);
	msg_info("status_fd %d %d", serv->status_fd[0], serv->status_fd[1]);
	msg_info("children: 0x%lx", (long) serv->children);
	msg_info("next: 0x%lx", (long) serv->next);
	msg_info("====end service entry");
}

/* free_master_ent - destroy process entry */

void free_master_ent(MASTER_SERV *serv)
{

	/*
	 * Undo what get_master_ent() created.
	 */
	if ( serv->host != NULL) 
	{
		myfree(serv->host);
	}
	myfree(serv->port);
	myfree(serv->name);
	myfree(serv->path);
	argv_free(serv->args);
	//myfree((char *) serv->listen_fd);
	myfree((char *) serv);
}
