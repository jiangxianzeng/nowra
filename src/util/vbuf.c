/*
 *
 * */

/* System library. */
#include <string.h>

/* Utility library. */
#include "vbuf.h"

/* vbuf_get - handle read buffer empty condition */

int vbuf_get(VBUF *bp)
{
    return (bp->get_ready(bp) ? VBUF_EOF : VBUF_GET(bp));
}

/* vbuf_put - handle write buffer full condition */

int vbuf_put(VBUF *bp, int ch)
{
    return (bp->put_ready(bp) ? VBUF_EOF : VBUF_PUT(bp, ch));
}

/* vbuf_read - bulk read from buffer */

int vbuf_read(VBUF *bp, char *buf, int len)
{
    int     count;
    char   *cp;
    int     n;

    for (cp = buf, count = len; count > 0; cp += n, count -= n) 
    {
        if (bp->cnt >= 0 && bp->get_ready(bp))
            break;
        n = (count < -bp->cnt ? count : -bp->cnt);
        memcpy(cp, bp->ptr, n);
        bp->ptr += n;
        bp->cnt += n;
    }
    return (len - count);
}

/* vbuf_write - bulk write to buffer */

int vbuf_write(VBUF *bp, const char *buf, int len)
{
    int     count;
    const char *cp;
    int     n;

    for (cp = buf, count = len; count > 0; cp += n, count -= n) 
    {
        if (bp->cnt <= 0 && bp->put_ready(bp) != 0)
            break;
        n = (count < bp->cnt ? count : bp->cnt);
        memcpy(bp->ptr, cp, n);
        bp->ptr += n;
        bp->cnt -= n;
    }
    return (len - count);
}


