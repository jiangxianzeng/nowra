/*
 *
 * */


/* System libraries. */

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

/* Application-specific. */

#include "msg.h"
#include "msg_output.h"


 /*
  *   * Default is verbose logging off.
  *     */
int     msg_verbose = 0;

static int msg_exiting = 0;

/* msg_info - report informative message */

void msg_info(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    msg_vprintf(MSG_INFO, fmt, ap);
    va_end(ap);
}

/* msg_debug - report debug message */

void msg_debug(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    msg_vprintf(MSG_DEBUG, fmt, ap);
    va_end(ap);
}

/* msg_warn - report warning message */

void msg_warn(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    msg_vprintf(MSG_WARN, fmt, ap);
    va_end(ap);
}

/* msg_error - report recoverable error */

void msg_error(const char *fmt,...)
{
    va_list ap;

    va_start(ap, fmt);
    msg_vprintf(MSG_ERROR, fmt, ap);
    va_end(ap);
    //if (++msg_error_count >= msg_error_bound)
    //    msg_fatal("too many errors - program terminated");
}

/* msg_fatal - report error and terminate gracefully */

void msg_fatal(const char *fmt,...)
{
    va_list ap;

    if (msg_exiting++ == 0) 
    {
        va_start(ap, fmt);
        msg_vprintf(MSG_FATAL, fmt, ap);
        va_end(ap);
   //     if (msg_cleanup_fn)
   //         msg_cleanup_fn();
    }
    sleep(1);
    abort();
    exit(1);
}

/* msg_panic - report error and dump core */

void msg_panic(const char *fmt,...)
{
    va_list ap;

    if (msg_exiting++ == 0) {
        va_start(ap, fmt);
        msg_vprintf(MSG_PANIC, fmt, ap);
        va_end(ap);
    }
    sleep(1);
    abort();                    /* Die! */
    exit(1);                    /* DIE!! */
}
