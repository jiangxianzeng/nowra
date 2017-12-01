#ifndef _MSG_OUTPUT_INCLUDE_
#define _MSG_OUTPUT_INCLUDE_

typedef void (*MSG_OUTPUT_FN) (int, const char *);
extern void msg_output(int, MSG_OUTPUT_FN);
extern void msg_printf(int, const char *,...);
extern void msg_vprintf(int, const char *, va_list);
extern void msg_text(int, const char *);


#define MSG_DEBUG   0	    /* debug */
#define MSG_INFO    1       /* informative */
#define MSG_WARN    2       /* warning (non-fatal) */
#define MSG_ERROR   3       /* error (fatal) */
#define MSG_FATAL   4       /* software error (fatal) */
#define MSG_PANIC   5       /* software error (fatal) */

#define MSG_LAST    5       /* highest-numbered severity level */

#endif 
