/*
 *
 * */


/* System libraries. */

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Application-specific. */

#include "msg_filelog.h"
#include "msg_output.h"


struct FILE_LOGGER {
    char *name;  /* log file name */
    int  level;  /* log level */
    int  fd;     /* log file descriptor */
    int  nerror; /* # log error */
};


static struct FILE_LOGGER file_logger;


#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define MAX(a, b)           ((a) > (b) ? (a) : (b))

/* msg_filelog_print - log info to syslog daemon */

static void msg_filelog_print(int level, const char *text)
{
	#define MAX_BUF_LEN 1280

	struct FILE_LOGGER *l = &file_logger;
	struct timeval tv;
	int len=0, size=0;
	ssize_t n = 0;

	char buf[MAX_BUF_LEN];
    if (l->fd < 0) 
    {
        return;
    }
	static char *severity_name[] = {
			"DEBUG","INFO", "WARNING", "ERROR", "FATAL", "PANIC",
			    };

    gettimeofday(&tv, NULL);
    size = MAX_BUF_LEN;
    len = strftime(buf, size-len, "[%Y-%m-%d %H:%M:%S.",  localtime(&tv.tv_sec));
    len += snprintf(buf+len, size-len, "%03ld] %s - %s\n", tv.tv_usec/1000, severity_name[level], text);

    n = write(l->fd, buf, len);

    if(n < 0)
    {
    	l->nerror++;
    }
}

int msg_filelog_init(int level, char *filename) 
{
    static int first_call = 1;

    struct FILE_LOGGER *l = &file_logger;

    l->level = MAX(MSG_INFO, MIN(level, MSG_LAST));
    l->name = filename;

    if (filename == NULL || !strlen(filename)) 
    {
        l->fd = STDERR_FILENO;
    } 
    else 
    {
        l->fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (l->fd < 0) 
        {
            printf("opening log file '%s' failed: %s", filename, strerror(errno));
            return -1;
        }
    }

    msg_output(l->level, msg_filelog_print);
    return 0;
}

void msg_filelog_free()
{
	struct FILE_LOGGER *l = &file_logger;

    if (l->fd < 0 || l->fd == STDERR_FILENO) {
        return;
    }

    close(l->fd);
}
