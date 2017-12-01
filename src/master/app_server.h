#ifndef _APP_SERVER_H_INCLUDE_
#define _APP_SERVER_H_INCLUDE_

 /*
  * External interface. Tables are defined in mail_conf.h.
  */
#define APP_SERVER_INT_TABLE   1
#define APP_SERVER_STR_TABLE   2
#define APP_SERVER_BOOL_TABLE  3
#define APP_SERVER_TIME_TABLE  4
#define APP_SERVER_RAW_TABLE   5

#define APP_SERVER_PRE_INIT    10
#define APP_SERVER_POST_INIT   11
#define APP_SERVER_LOOP    12
#define APP_SERVER_EXIT    13
#define APP_SERVER_PRE_ACCEPT  14

typedef void (*APP_SERVER_INIT_FN) (char *, char **);
typedef int (*APP_SERVER_LOOP_FN) (char *, char **);
typedef void (*APP_SERVER_EXIT_FN) (char *, char **);
typedef void (*APP_SERVER_ACCEPT_FN) (char *, char **);

/*
 * single_server.c
 */
typedef void (*SINGLE_SERVER_FN) (int, char *, char **);
extern void single_server_main(int, char **, SINGLE_SERVER_FN, ...);

#endif
