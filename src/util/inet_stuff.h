#ifndef _INET_STUFF_H_INCLUDE_
#define _INET_STUFF_H_INCLUDE_

/*
 *
 * */


extern int inet_connect(const char *, int, int);

extern int inet_listen(const char *addr, int backlog, int block_mode);

extern int inet_accept(int);

#endif
