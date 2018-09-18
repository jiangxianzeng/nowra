/* System library. */

#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "vbuf_print.h"
#include "iostuff.h"
#include "vstring.h"
#include "vstream.h"

/* Application-specific. */

 /*
 * Forward declarations.
 */
static int vstream_buf_get_ready(VBUF *);
static int vstream_buf_put_ready(VBUF *);
static int vstream_buf_space(VBUF *, int);

 /*
  * A bunch of macros to make some expressions more readable. XXX We're
  * assuming that O_RDONLY == 0, O_WRONLY == 1, O_RDWR == 2.
  */
#define VSTREAM_ACC_MASK(f) ((f) & (O_APPEND | O_WRONLY | O_RDWR))

#define VSTREAM_CAN_READ(f) (VSTREAM_ACC_MASK(f) == O_RDONLY \
                || VSTREAM_ACC_MASK(f) == O_RDWR)
#define VSTREAM_CAN_WRITE(f)    (VSTREAM_ACC_MASK(f) & O_WRONLY \
                || VSTREAM_ACC_MASK(f) & O_RDWR \
                || VSTREAM_ACC_MASK(f) & O_APPEND)



#define VSTREAM_BUF_ZERO(bp) { \
    (bp)->flags = 0; \
    (bp)->data = (bp)->ptr = 0; \
    (bp)->len = (bp)->cnt = 0; \
    }

#define VSTREAM_BUF_ACTIONS(bp, get_action, put_action, space_action) { \
    (bp)->get_ready = (get_action); \
    (bp)->put_ready = (put_action); \
    (bp)->space = (space_action); \
    }


/* vstream_buf_init - initialize buffer */

static void vstream_buf_init(VBUF *bp, int flags)
{

    /*
     * Initialize the buffer such that the first data access triggers a
     * buffer boundary action.
     */
    VSTREAM_BUF_ZERO(bp);
    VSTREAM_BUF_ACTIONS(bp,
            VSTREAM_CAN_READ(flags) ? vstream_buf_get_ready : 0,
            VSTREAM_CAN_WRITE(flags) ? vstream_buf_put_ready : 0,
            vstream_buf_space);
}

/* vstream_fdopen - add buffering to pre-opened stream */

VSTREAM *vstream_fdopen(int fd, int flags)
{
    VSTREAM *stream;

    /*
     * Sanity check.
     */
    if (fd < 0) {
        msg_panic("vstream_fdopen: bad file %d", fd);
    }
    
    /*
     * Initialize buffers etc. but do as little as possible. Late buffer
     * allocation etc. gives the application a chance to override default
     * policies. Either this, or the vstream*open() routines would have to
     * have a really ugly interface with lots of mostly-unused arguments (can
     * you say VMS?).
     */
    stream = (VSTREAM *) mymalloc(sizeof(*stream));
    stream->fd = fd;
    stream->read_fn = VSTREAM_CAN_READ(flags) ? (VSTREAM_FN) timed_read : 0;
    stream->write_fn = VSTREAM_CAN_WRITE(flags) ? (VSTREAM_FN) timed_write : 0;
    vstream_buf_init(&stream->buf, flags);
    stream->offset = 0;
    stream->path = 0;
    stream->pid = 0;
    stream->waitpid_fn = 0;
    stream->timeout = 0;
    stream->context = 0;
    stream->jbuf = 0;
    stream->iotime = 0;
    return (stream);

}
/* vstream_fopen - open buffered file stream */

VSTREAM *vstream_fopen(const char *path, int flags, int mode)
{
    VSTREAM *stream;
    int     fd;

    if ((fd = open(path, flags, mode)) < 0) {
    return (0);
    } else {
    stream = vstream_fdopen(fd, flags);
    stream->path = mystrdup(path);
    return (stream);
    }
}
