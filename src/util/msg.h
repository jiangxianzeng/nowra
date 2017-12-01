#ifndef _MSG_H_INCLUDED_
#define _MSG_H_INCLUDED_

/*
 * External interface.
 */
//typedef void (*MSG_CLEANUP_FN) (void);

extern int msg_verbose;

extern void msg_info(const char *,...);
extern void msg_debug(const char *, ...);
extern void msg_warn(const char *,...);
extern void msg_error(const char *,...);
extern void msg_fatal(const char *,...);
extern void msg_panic(const char *,...);

//extern int msg_error_limit(int);
//extern void msg_error_clear(void);
//extern MSG_CLEANUP_FN msg_cleanup(MSG_CLEANUP_FN);

#endif
