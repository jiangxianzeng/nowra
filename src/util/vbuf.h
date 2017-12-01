#ifndef _VBUF_H_INCLUDED_                                                                                                   
#define _VBUF_H_INCLUDED_
/*
 *
 *
 * */

typedef struct VBUF VBUF;
typedef int (*VBUF_GET_READY_FN) (VBUF *);
typedef int (*VBUF_PUT_READY_FN) (VBUF *);
typedef int (*VBUF_SPACE_FN) (VBUF *, size_t);

struct VBUF {
    int     flags;              /* status, see below */
    unsigned char *data;        /* variable-length buffer */
    size_t len;                /* buffer length */
    size_t cnt;                /* bytes left to read/write */
    unsigned char *ptr;         /* read/write position */
    VBUF_GET_READY_FN get_ready;    /* read buffer empty action */      
    VBUF_PUT_READY_FN put_ready;    /* write buffer full action */
    VBUF_SPACE_FN space;        /* request for buffer space */
};


#define VBUF_TO_APPL(vbuf_ptr,app_type,vbuf_member) \
        ((app_type *) (((char *) (vbuf_ptr)) - offsetof(app_type,vbuf_member)))

 /*
  *   * Buffer status management.
  * */
#define VBUF_FLAG_ERR   (1<<0)      /* some I/O error */
#define VBUF_FLAG_EOF   (1<<1)      /* end of data */
#define VBUF_FLAG_TIMEOUT (1<<2)    /* timeout error */
#define VBUF_FLAG_BAD   (VBUF_FLAG_ERR | VBUF_FLAG_EOF | VBUF_FLAG_TIMEOUT)
#define VBUF_FLAG_FIXED (1<<3)      /* fixed-size buffer */

#define vbuf_error(v)   ((v)->flags & VBUF_FLAG_ERR)
#define vbuf_eof(v) ((v)->flags & VBUF_FLAG_EOF)
#define vbuf_timeout(v) ((v)->flags & VBUF_FLAG_TIMEOUT)
#define vbuf_clearerr(v) ((v)->flags &= ~VBUF_FLAG_BAD)

 /*
  *   * Buffer I/O-like operations and results.
  *     */
#define VBUF_GET(v) ((v)->cnt < 0 ? ++(v)->cnt, \
        (int) *(v)->ptr++ : vbuf_get(v))
#define VBUF_PUT(v,c)   ((v)->cnt > 0 ? --(v)->cnt, \
        (int) (*(v)->ptr++ = (c)) : vbuf_put((v),(c)))
#define VBUF_SPACE(v,n) ((v)->space((v),(n)))

#define VBUF_EOF    (-1)        /* no more space or data */

extern int vbuf_get(VBUF *);
extern int vbuf_put(VBUF *, int);
//extern int vbuf_unget(VBUF *, int);
extern int vbuf_read(VBUF *, char *, int);
extern int vbuf_write(VBUF *, const char *, int);


#endif
