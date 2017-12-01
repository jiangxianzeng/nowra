#ifndef _VSTRING_H_INCLUDED_
#define _VSTRING_H_INCLUDED_

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include "vbuf.h"

typedef struct VSTRING {
    VBUF    vbuf;
    int     maxlen;
} VSTRING;

extern VSTRING *vstring_alloc(int);
extern void vstring_ctl(VSTRING *,...);
extern VSTRING *vstring_truncate(VSTRING *, int);
extern VSTRING *vstring_free(VSTRING *);
extern VSTRING *vstring_strcpy(VSTRING *, const char *);
extern VSTRING *vstring_strncpy(VSTRING *, const char *, int);
extern VSTRING *vstring_memcpy(VSTRING *, const char *, ssize_t);
extern VSTRING *vstring_memcat(VSTRING *, const char *, ssize_t);
extern char *vstring_memchr(VSTRING *, int);
extern VSTRING *vstring_strcat(VSTRING *, const char *);
extern VSTRING *vstring_strncat(VSTRING *, const char *, int);
extern VSTRING *vstring_sprintf(VSTRING *, const char *,...);
extern VSTRING *vstring_vsprintf(VSTRING *, const char *, va_list);

extern char *vstring_export(VSTRING *);
extern VSTRING *vstring_import(char *);

extern void vstring_range(VSTRING *vp, int start, int end);

#define VSTRING_CTL_MAXLEN  1
#define VSTRING_CTL_END     0

 /*
  *   * Macros. Unsafe macros have UPPERCASE names.
  *     */
#define VSTRING_SPACE(vp, len)  ((vp)->vbuf.space(&(vp)->vbuf, len))
#define VSTRING_STR(vp)     ((char *) (vp)->vbuf.data)
#define VSTRING_LEN(vp)     ((vp)->vbuf.ptr - (vp)->vbuf.data)
#define VSTRING_END(vp)     ((char *) (vp)->vbuf.ptr)
#define VSTRING_TERMINATE(vp)   { if ((vp)->vbuf.cnt <= 0) \
                        VSTRING_SPACE((vp),1); \
                      *(vp)->vbuf.ptr = 0; }
#define VSTRING_RESET(vp)   { (vp)->vbuf.ptr = (vp)->vbuf.data; \
                      (vp)->vbuf.cnt = (vp)->vbuf.len; }
#define VSTRING_ADDCH(vp, ch)   VBUF_PUT(&(vp)->vbuf, ch)
#define VSTRING_SKIP(vp)    { while ((vp)->vbuf.cnt > 0 && *(vp)->vbuf.ptr) \
                      (vp)->vbuf.ptr++, (vp)->vbuf.cnt--; }
#define VSTRING_AVAIL(vp)   ((vp)->vbuf.cnt)

#define VSTRING_INCLEN(vp, incr) {(vp)->vbuf.ptr += incr; (vp)->vbuf.cnt -= incr;}
 /*
  *   * The following macro is not part of the public interface, because it can
  *     * really screw up a buffer by positioning past allocated memory.
  *       */
#define VSTRING_AT_OFFSET(vp, offset) { \
        (vp)->vbuf.ptr = (vp)->vbuf.data + (offset); \
        (vp)->vbuf.cnt = (vp)->vbuf.len - (offset); \
        }
#endif
