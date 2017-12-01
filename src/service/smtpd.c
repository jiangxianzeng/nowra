/*
 * @Name: smtpd.c
 *
 */
#include <stdio.h>

#include "msg.h"
#include "app_server.h"
#include "vstring.h"
#include "iostuff.h"

static void handle_service(int fd, char *unused_service, char **argv)
{
	char buf[1024] = {0};
	int len = 1024;
	timed_read(fd, buf, len, 0, NULL);
	printf("read buf:%s.\n", buf);
}

/* main - pass control to the single-threaded skeleton */

int main(int argc, char **argv)
{
	single_server_main(argc, argv, handle_service,0);
}
