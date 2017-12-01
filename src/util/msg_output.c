/*
 *
 * */

/* System library. */
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

/* Utility library. */
#include "msg_output.h"
#include "mymalloc.h"

 /*
  * Private state.
  */
static MSG_OUTPUT_FN msg_output_fn = 0;
static char *msg_buffer = 0;
static int msg_level =0;

/* msg_output  - specify output handler */

void msg_output(int level, MSG_OUTPUT_FN output_fn)
{
 // if( msg_output_fn == 0)
 // {
 //     msg_output_fn = (MSG_OUTPUT_FN *) mymalloc(sizeof(*msg_output_fn));
 // }
	
  msg_output_fn = output_fn;
  msg_level = level;
}

/* msg_printf - format text and log it */

void msg_printf(int level, const char *format,...)
{
    if(level < msg_level)
	return;

    va_list ap;

    va_start(ap, format);
    msg_vprintf(level, format, ap);
    va_end(ap);
}

/* msg_vprintf - format text and log it */

void msg_vprintf(int level, const char *format, va_list ap)
{
   if (msg_buffer == 0)
        msg_buffer = mymalloc(1024);
   else
       memset(msg_buffer, 0, 1024);

   vsnprintf(msg_buffer, 1023, format, ap);
  
   msg_text(level, msg_buffer); 
}

/* msg_text - sanitize and log pre-formatted text */

void msg_text(int level, const char *text)
{
	if(msg_output_fn == 0)
	{
    	printf("%d, %s\n", level, text);
	}
	else
	{
    	msg_output_fn(level, text);
	}
}
