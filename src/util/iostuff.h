#ifndef _IOSTUFF_H_INCLUDED_
#define _IOSTUFF_H_INCLUDED_

extern int non_blocking(int fd, int on);
extern int non_delaying(int fd, int on);
extern int close_on_exec(int fd, int on);

extern int read_wait(int fd, int timeout);
extern int write_wait(int fd, int timeout);

extern int readable(int fd);
extern int writable(int fd);
extern size_t write_buf(int, const char *, size_t, int);
extern size_t timed_read(int, void *, size_t, int, void *);
extern size_t timed_write(int, void *, size_t, int, void *);

#define BLOCKING    0
#define NON_BLOCKING    1

#define TCP_DELAYING 0
#define TCP_NO_DELAYING 1

#define CLOSE_ON_EXEC   1
#define PASS_ON_EXEC    0

#endif
