/*
 *
 * */

/* System libraries. */

#include <stddef.h>
#include <stdlib.h>         /* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "vstring.h"


/* vstring_extend - variable-length string buffer extension policy */

static void vstring_extend(VBUF *bp, int incr)
{
    unsigned used = bp->ptr - bp->data;

    bp->len += (bp->len > incr ? bp->len : incr);
    bp->data = (unsigned char *) myrealloc((char *) bp->data, bp->len);
    bp->ptr = bp->data + used;
    bp->cnt = bp->len - used;
}

/* vstring_buf_get_ready - vbuf callback for read buffer empty condition */

static int vstring_buf_get_ready(VBUF *unused_buf)
{
    msg_panic("vstring_buf_get: write-only buffer");
}

/* vstring_buf_put_ready - vbuf callback for write buffer full condition */

static int vstring_buf_put_ready(VBUF *bp)
{
    vstring_extend(bp, 0);
    return (0);
}

/* vstring_buf_space - vbuf callback to reserve space */

static int vstring_buf_space(VBUF *bp, size_t len)
{
    int need;

    if (len < 0)
        msg_panic("vstring_buf_space: bad length %d", len);
    if ((need = len - bp->cnt) > 0)
        vstring_extend(bp, need);
    return (0);
}

/* vstring_alloc - create variable-length string */

VSTRING *vstring_alloc(int len)
{
    VSTRING *vp;

    if (len < 1)
        msg_panic("vstring_alloc: bad length %d", len);
    vp = (VSTRING *) mymalloc(sizeof(*vp));
    vp->vbuf.flags = 0;
    vp->vbuf.data = (unsigned char *) mymalloc(len);
    vp->vbuf.len = len;
    VSTRING_RESET(vp);
    vp->vbuf.data[0] = 0;
    vp->vbuf.get_ready = vstring_buf_get_ready;
    vp->vbuf.put_ready = vstring_buf_put_ready;
    vp->vbuf.space = vstring_buf_space;
    vp->maxlen = 0;
    return (vp);
}

/* vstring_free - destroy variable-length string */

VSTRING *vstring_free(VSTRING *vp)
{
    if (vp->vbuf.data)
        myfree((char *) vp->vbuf.data);
    myfree((char *) vp);
    return (0);
}

/* vstring_ctl - modify memory management policy */

void vstring_ctl(VSTRING *vp,...)
{
    va_list ap;
    int     code;

    va_start(ap, vp);
    while ((code = va_arg(ap, int)) != VSTRING_CTL_END) 
    {
        switch (code) 
        {
            default:
                msg_panic("vstring_ctl: unknown code: %d", code);
            case VSTRING_CTL_MAXLEN:
                vp->maxlen = va_arg(ap, int);
                if (vp->maxlen < 0)
                    msg_panic("vstring_ctl: bad max length %d", vp->maxlen);
                break;
        }
    }
    va_end(ap);
}

/* vstring_truncate - truncate string */

VSTRING *vstring_truncate(VSTRING *vp, int len)
{
    if (len < 0)
        msg_panic("vstring_truncate: bad length %d", len);
    if (len < VSTRING_LEN(vp))
        VSTRING_AT_OFFSET(vp, len);
    return (vp);
}

/* vstring_strcpy - copy string */

VSTRING *vstring_strcpy(VSTRING *vp, const char *src)
{
    VSTRING_RESET(vp);

    while (*src) 
    {
        VSTRING_ADDCH(vp, *src);
        src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_strncpy - copy string of limited length */

VSTRING *vstring_strncpy(VSTRING *vp, const char *src, int len)
{
    VSTRING_RESET(vp);

    while (len-- > 0 && *src) 
    {
        VSTRING_ADDCH(vp, *src);
        src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_memcpy - copy buffer of limited length */

VSTRING *vstring_memcpy(VSTRING *vp, const char *src, ssize_t len)
{
    VSTRING_RESET(vp);

    VSTRING_SPACE(vp, len);
    memcpy(VSTRING_STR(vp), src, len);
    VSTRING_AT_OFFSET(vp, len);
    return (vp);
}

/* vstring_memcat - append buffer of limited length */

VSTRING *vstring_memcat(VSTRING *vp, const char *src, ssize_t len)
{
    VSTRING_SPACE(vp, len);
    memcpy(VSTRING_END(vp), src, len);
    len += VSTRING_LEN(vp);
    VSTRING_AT_OFFSET(vp, len);
    return (vp);
}


/* vstring_memchr - locate byte in buffer */

char *vstring_memchr(VSTRING *vp, int ch)
{
    unsigned char *cp;

    for (cp = (unsigned char *) VSTRING_STR(vp); cp < (unsigned char *) VSTRING_END(vp); cp++)
        if (*cp == ch)
            return ((char *) cp);
    return (0);
}

/* vstring_strcat - append string */

VSTRING *vstring_strcat(VSTRING *vp, const char *src)
{
    while (*src) 
    {
        VSTRING_ADDCH(vp, *src);
        src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_strncat - append string of limited length */

VSTRING *vstring_strncat(VSTRING *vp, const char *src, int len)
{
    while (len-- > 0 && *src) 
    {
        VSTRING_ADDCH(vp, *src);
        src++;
    }
    VSTRING_TERMINATE(vp);
    return (vp);
}

/* vstring_sprintf - formatted string */

VSTRING *vstring_sprintf(VSTRING *vp, const char *format,...)
{
	va_list ap;

	va_start(ap, format);
	vp = vstring_vsprintf(vp, format, ap);
	va_end(ap);
	return (vp);
}

/* vstring_vsprintf - format string, vsprintf-like interface */

VSTRING *vstring_vsprintf(VSTRING *vp, const char *format, va_list ap)
{
	VSTRING_RESET(vp);
	vbuf_print(&vp->vbuf, format, ap);
	VSTRING_TERMINATE(vp);
	return (vp);
}

/* vstring_export - VSTRING to bare string */

char *vstring_export(VSTRING *vp)
{
    char   *cp;

    cp = (char *) vp->vbuf.data;
    vp->vbuf.data = 0;
    myfree((char *) vp);
    return (cp);
}

/* vstring_import - bare string to vstring */

VSTRING *vstring_import(char *str)
{
    VSTRING *vp;
    int     len;

    vp = (VSTRING *) mymalloc(sizeof(*vp));
    len = strlen(str);
    vp->vbuf.data = (unsigned char *) str;
    vp->vbuf.len = len + 1;
    VSTRING_AT_OFFSET(vp, len);
    vp->maxlen = 0;
    return (vp);
}


void vstring_range(VSTRING *vp, int start, int end)
{
    size_t newlen, len;
    len = VSTRING_LEN(vp);

    if (len == 0) return;

    if (start < 0) {
        start = len + start;
        if (start < 0) start = 0;
    }

    if (end < 0) {
        end  = len + end;
        if (end < 0) end = 0;
    }

    newlen = (start > end) ? 0 : (end - start)+1;

    if (newlen != 0) {
        if (start >= (signed)len) {
            newlen = 0;
        } else if (end >= (signed)len) {
            end = len-1;
            newlen = (start > end) ? 0 : (end-start)+1;
        }
    } else {
        start = 0;
    }
    if (start && newlen) {
        memmove(VSTRING_STR(vp), VSTRING_STR(vp)+start, newlen);
        VSTRING_INCLEN(vp, -start);
    } else {
        VSTRING_RESET(vp);
    }
	VSTRING_TERMINATE(vp);
}
