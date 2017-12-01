/*
 *
 * */


/* System libraries. */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>
#include <errno.h>

/* Application-specific. */

#include "inet_stuff.h"
#include "msg.h"
#include "iostuff.h"

/* sane_connect - sanitize connect() results */

static int sane_connect(int sock, struct sockaddr * sa, size_t len)
{
    if (sa->sa_family == AF_INET) 
    {
        int on = 1;

        (void) setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
                (char *) &on, sizeof(on));
    }
    return (connect(sock, sa, len));
}
/* timed_connect - connect with deadline */

static int timed_connect(int sock, struct sockaddr * sa, int len, int timeout)
{
    const char *myname = "timed_connect";

    int    error;
    socklen_t error_len;

    if(timeout <=0 )
    {
        msg_error("%s: bad timeout: %d",myname, timeout);
        return -1;
    }
    /*
     * Start the connection, and handle all possible results.
     */
    if (sane_connect(sock, sa, len) == 0)
    {
        return (0);
    }
    if (errno != EINPROGRESS)
    {
        return (-1);
    }
    /*
     * A connection is in progress. Wait for a limited amount of time for
     * something to happen. If nothing happens, report an error.
     */
    if (write_wait(sock, timeout) < 0)
        return (-1);

    /*
     * Something happened. Some Solaris 2 versions have getsockopt() itself
     * return the error, instead of returning it via the parameter list.
     */
    error = 0;
    error_len = sizeof(error);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &error, &error_len) < 0)
        return (-1);
    if (error) 
    {
        errno = error;
        return (-1);
    }

    /*
     * No problems.
     */
    return (0);

}

/* inet_connect - connect to AF_INET-domain listener */

int inet_connect(const char *addr, int block_mode, int timeout)
{
    const char *myname = "inet_connect";

    char   buf[32] = {0};
    char   *host;
    char   *port;
    struct sockaddr_in sin;
    int     sock;

    if(addr == NULL)
    {
        msg_error("%s: addr is NULL.",myname);
        return -1;
    }
    strncpy(buf, addr, 31);

    if((port = strrchr(buf, ':')) != NULL)
    {
        host = buf;
        *port = '\0';
        port++;
    }
    else
    {
        msg_error("%s: parse host or port failed.addr:%s.",myname, addr);
        return -1;
    }
    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    sin.sin_addr.s_addr = inet_addr(host);
    sin.sin_port = htons(atoi(port));


    /*
     * Create a client socket.
     */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        msg_fatal("%s: socket: %s.",myname, strerror(errno));
    }

    /*
     * Timed connect.
     */
    if (timeout > 0) 
    {
        non_blocking(sock, NON_BLOCKING);
        if (timed_connect(sock, (struct sockaddr *) &sin, sizeof(sin), timeout) < 0) 
        {
            close(sock);
            return (-1);
        }

        if (block_mode != NON_BLOCKING)
            non_blocking(sock, block_mode);
        return (sock);
    }
    else/* Maybe block until connected. */ 
    {
        non_blocking(sock, block_mode);
        if (sane_connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0
                && errno != EINPROGRESS) 
        {
            close(sock);
            return (-1);
        }
        return (sock);
    }
}

/* inet_listen - create TCP listener */

int inet_listen(const char *addr, int backlog, int block_mode)
{
    const char *myname = "inet_listen";

    struct sockaddr_in sin;
    int     sock;
    int     t = 1;
    char   buf[32] = {0};
    char   *host;
    char   *port;
    
    if(addr == NULL)
    {
        msg_error("%s: addr is NULL.",myname);
        return -1;
    }
    strncpy(buf, addr, 31);

    if((port = strrchr(buf, ':')) != NULL)
    {
        host = buf;
        *port = '\0';
        port++;
    }
    else
    {
        msg_error("%s: parse host or port failed.addr:%s.",myname, addr);
        return -1;
    }

    memset((char *) &sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(atoi(port));
    sin.sin_addr.s_addr = (*host ? inet_addr(host): INADDR_ANY);

    /*
     * Create a listener socket.
     */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        msg_fatal("%s: socket: %s.",myname, strerror(errno));
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &t, sizeof(t)) < 0)
    {
        msg_fatal("%s: setsockopt: %s.",myname, strerror(errno));
    }

    if (bind(sock, (struct sockaddr *) & sin, sizeof(sin)) < 0)
    {
        msg_fatal("%s: bind %s port %d: %s.",myname, sin.sin_addr.s_addr == INADDR_ANY ?
                "INADDR_ANY" : inet_ntoa(sin.sin_addr), ntohs(sin.sin_port), strerror(errno));
    }
    non_blocking(sock, block_mode);
    if (listen(sock, backlog) < 0)
    {
        msg_fatal("%s: listen: %s.", myname, strerror(errno));
    }
    return sock;
}

/* sane_accept - sanitize accept() error returns */

int sane_accept(int sock, struct sockaddr * sa, socklen_t *len)
{
    const char *myname = "sane_accept";

    static int accept_ok_errors[] = {
        EAGAIN,
        ECONNREFUSED,
        ECONNRESET,
        EHOSTDOWN,
        EHOSTUNREACH,
        EINTR,
        ENETDOWN,
        ENETUNREACH,
        ENOTCONN,
        EWOULDBLOCK,
        0,
    };
    int     count;
    int     err;
    int     fd;
	//msg_debug("%s: sock:%d.", myname, sock);
    if ((fd = accept(sock, sa, len)) < 0) 
    {
		msg_error("%s: accept error,%s.", myname, strerror(errno));
        for (count = 0; (err = accept_ok_errors[count]) != 0; count++) 
        {
            if (errno == err) 
            {
                errno = EAGAIN;
                break;
            }
        }
    }
    return (fd);
}
/* inet_accept - accept connection */

int inet_accept(int fd)
{
    struct sockaddr_in inaddr;
    socklen_t ss_len;

    return (sane_accept(fd, (struct sockaddr *) &inaddr, &ss_len));
}
