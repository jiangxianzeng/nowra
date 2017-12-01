/*
 *
 *
 * */


/* System interfaces. */
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/* Utility library. */

#include "msg.h"
#include "iostuff.h"

/* non_blocking - set/clear non-blocking flag */

int non_blocking(int fd, int on)
{
    int     flags;

    if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
        msg_fatal("fcntl: get flags: %s.", strerror(errno));
    if (fcntl(fd, F_SETFL, on ? flags | O_NONBLOCK : flags & ~O_NONBLOCK) < 0)
        msg_fatal("fcntl: set non-blocking flag %s: %s.", on ? "on" : "off", strerror(errno));
    return ((flags & O_NONBLOCK) != 0);
}

/*non_delaying - */
int non_delaying(int fd, int on)
{
    char *myname = "non_delaying";
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1)
    {
        msg_error("%s: setsockopt TCP_NODELAY: %s",myname, strerror(errno));
        return -1;
    }
    return 0;

}

/* close_on_exec - set/clear close-on-exec flag */

int close_on_exec(int fd, int on)
{
    int flags;

    if ((flags = fcntl(fd, F_GETFD, 0)) < 0)
        msg_fatal("fcntl: get flags: %s.",strerror(errno));
    if (fcntl(fd, F_SETFD, on ? flags | FD_CLOEXEC : flags & ~FD_CLOEXEC) < 0)
        msg_fatal("fcntl: set close-on-exec flag %s: %s.", on ? "on" : "off", strerror(errno));
    return ((flags & FD_CLOEXEC) != 0);
}


/* write_wait - block with timeout until file descriptor is writable */

int write_wait(int fd, int timeout)
{
    const char *myname = "write_wait";

    struct pollfd pollfd;

#define WAIT_FOR_EVENT  (-1)
    pollfd.fd = fd;
    pollfd.events = POLLOUT;
    for (;;) 
    {
        switch (poll(&pollfd, 1, timeout < 0 ?  WAIT_FOR_EVENT : timeout * 1000)) 
        {
            case -1:
                if (errno != EINTR)
                    msg_fatal("%s: poll: %s",myname, strerror(errno));
                continue;
            case 0:
                errno = ETIMEDOUT;
                return (-1);
            default:
                if (pollfd.revents & POLLNVAL)
                    msg_fatal("%s: poll: %s",myname, strerror(errno));
                return (0);
        }
    }
    return 0;
}

/* read_wait - block with timeout until file descriptor is readable */

int read_wait(int fd, int timeout)
{
    const char *myname = "read_wait";

    struct pollfd pollfd;

#define WAIT_FOR_EVENT  (-1)

    pollfd.fd = fd;
    pollfd.events = POLLIN;

    for (;;) 
    {
        switch (poll(&pollfd, 1, timeout < 0 ? WAIT_FOR_EVENT : timeout * 1000)) 
        {
            case -1:
                if (errno != EINTR)
                    msg_fatal("%s: poll: %s.",myname, strerror(errno));
                continue;
            case 0:
                errno = ETIMEDOUT;
                return (-1);
            default:
                if (pollfd.revents & POLLNVAL)
                    msg_fatal("%s: poll: %s.",myname, strerror(errno));
                return (0);
        }
    }
}

/* readable - see if file descriptor is readable */

int readable(int fd)
{
    const char *myname = "readable";

    struct pollfd pollfd;

#define DONT_WAIT_FOR_EVENT 0

    pollfd.fd = fd;
    pollfd.events = POLLIN;
    for (;;) 
    {
        switch (poll(&pollfd, 1, DONT_WAIT_FOR_EVENT)) 
        {
            case -1:
                if (errno != EINTR)
                    msg_fatal("%s: poll: %s.",myname, strerror(errno));
                continue;
            case 0:
                return (0);
            default:
                if (pollfd.revents & POLLNVAL)
                    msg_fatal("%s: poll: %s.",myname, strerror(errno));
                return (1);
        }
    }
}


/* writable - see if file descriptor is writable */

int writable(int fd)
{
    const char *myname = "writable";

    struct pollfd pollfd;

#define DONT_WAIT_FOR_EVENT 0

    pollfd.fd = fd;
    pollfd.events = POLLOUT;
    for (;;) 
    {
        switch (poll(&pollfd, 1, DONT_WAIT_FOR_EVENT)) 
        {
            case -1:
                if (errno != EINTR)
                    msg_fatal("%s: poll: %s.",myname, strerror(errno));
                continue;
            case 0:
                return (0);
            default:
                if (pollfd.revents & POLLNVAL)
                    msg_fatal("%s: poll: %s.",myname, strerror(errno));
                return (1);
        }
    }
}

/* write_buf - write buffer or bust,mybe use epoll ET model */

size_t write_buf(int fd, const char *buf, size_t len, int timeout)
{
    const char *myname = "write_buf";

    const char *start = buf;
    size_t count;
    time_t  expire;
    int     time_left = timeout;

    if (time_left > 0)
        expire = time((time_t *) 0) + time_left;

    while (len > 0) 
    {
        if (time_left > 0 && write_wait(fd, time_left) < 0)
            return (-1);
        if ((count = write(fd, buf, len)) < 0) 
        {
            if ((errno == EAGAIN && time_left > 0) || errno == EINTR)
                /* void */ ;
            else
                return (-1);
        } else {
            buf += count;
            len -= count;
        }
        if (len > 0 && time_left > 0) {
            time_left = expire - time((time_t *) 0);
            if (time_left <= 0) {
                errno = ETIMEDOUT;
                return (-1);
            }
        }
    }
    return (buf - start);
}

/* timed_read - read with deadline , mybe use epoll LT model*/

size_t timed_read(int fd, void *buf, size_t len,
                           int timeout, void *unused_context)
{
    const char *myname = "timed_read";
    size_t ret;

    for (;;) 
    {
        if (timeout > 0 && read_wait(fd, timeout) < 0)
            return (-1);
        if ((ret = read(fd, buf, len)) < 0 && timeout > 0 && errno == EAGAIN) 
        {
            msg_warn("read() returns EAGAIN on a readable file descriptor!");
            msg_warn("pausing to avoid going into a tight select/read loop!");
            sleep(1);
            continue;
        } 
        else if (ret < 0 && errno == EINTR) 
        {
            continue;
        } 
        else 
        {
            return (ret);
        }
    }

}

/* timed_write - write with deadline , mybe use epoll LT model*/

size_t timed_write(int fd, void *buf, size_t len,
                    int timeout, void *unused_context)
{
    const char *myname = "timed_write";
    size_t ret;
   
    for (;;) 
    {
        if (timeout > 0 && write_wait(fd, timeout) < 0)
            return (-1);
        if ((ret = write(fd, buf, len)) < 0 && timeout > 0 && errno == EAGAIN) 
        {
            msg_warn("write() returns EAGAIN on a writable file descriptor!");
            msg_warn("pausing to avoid going into a tight select/write loop!");
            sleep(1);
            continue;
        } 
        else if (ret < 0 && errno == EINTR) 
        {
            continue;
        } 
        else 
        {
            return (ret);
        }
    }
}
