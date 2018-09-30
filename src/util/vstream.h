#ifndef _VSTREAM_H_INCLUDED_
#define _VSTREAM_H_INCLUDED_

/*++
/* NAME
/*	vstream 3h
/* SUMMARY
/*	simple buffered I/O package


 /*
  * System library.
  */
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <setjmp.h>

 /*
  * Utility library.
  */
#include <vbuf.h>

 /*
  * Simple buffered stream. The members of this structure are not part of the
  * official interface and can change without prior notice.
  */
typedef int (*VSTREAM_FN) (int, void *, unsigned, int, void *);
typedef int (*VSTREAM_WAITPID_FN) (pid_t, int *, int);

typedef struct VSTREAM {
    VBUF    buf;            /* generic intelligent buffer */
    int     fd;             /* file handle, no 256 limit */
    VSTREAM_FN read_fn;         /* buffer fill action */
    VSTREAM_FN write_fn;        /* buffer fill action */
    void   *context;            /* application context */
    long    offset;         /* cached seek info */
    char   *path;           /* give it at least try */
    int     read_fd;            /* read channel (double-buffered) */
    int     write_fd;           /* write channel (double-buffered) */
    VBUF    read_buf;           /* read buffer (double-buffered) */
    VBUF    write_buf;          /* write buffer (double-buffered) */
    pid_t   pid;            /* vstream_popen/close() */
    VSTREAM_WAITPID_FN waitpid_fn;  /* vstream_popen/close() */
    int     timeout;            /* read/write timout */
    jmp_buf *jbuf;          /* exception handling */
    time_t  iotime;         /* time of last fill/flush */
} VSTREAM;

extern VSTREAM vstream_fstd[];      /* pre-defined streams */

#define VSTREAM_IN      (&vstream_fstd[0])
#define VSTREAM_OUT     (&vstream_fstd[1])
#define VSTREAM_ERR     (&vstream_fstd[2])

#define VSTREAM_FLAG_ERR    VBUF_FLAG_ERR   /* some I/O error */
#define VSTREAM_FLAG_EOF    VBUF_FLAG_EOF   /* end of file */
#define VSTREAM_FLAG_TIMEOUT    VBUF_FLAG_TIMEOUT   /* timeout error */
#define VSTREAM_FLAG_FIXED  VBUF_FLAG_FIXED /* fixed-size buffer */
#define VSTREAM_FLAG_BAD    VBUF_FLAG_BAD

#define VSTREAM_FLAG_READ   (1<<8)  /* read buffer */
#define VSTREAM_FLAG_WRITE  (1<<9)  /* write buffer */
#define VSTREAM_FLAG_SEEK   (1<<10) /* seek info valid */
#define VSTREAM_FLAG_NSEEK  (1<<11) /* can't seek this file */
#define VSTREAM_FLAG_DOUBLE (1<<12) /* double buffer */

#define VSTREAM_BUFSIZE     4096

extern VSTREAM *vstream_fopen(const char *, int, int);
extern VSTREAM *vstream_fdopen(int, int);
extern int vstream_fclose(VSTREAM *);

#endif