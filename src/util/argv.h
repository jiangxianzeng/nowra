#ifndef _ARGV_H_INCLUDED_
#define _ARGV_H_INCLUDED_

 /*
  * * External interface.
  **/
typedef struct ARGV {
    int     len;            /* number of array elements */
    int     argc;           /* array elements in use */
    char  **argv;           /* string array */
} ARGV;

extern ARGV *argv_alloc(int);
extern void argv_add(ARGV *,...);
extern void argv_terminate(ARGV *);
extern ARGV *argv_free(ARGV *);

extern ARGV *argv_split(const char *, const char *);
extern ARGV *argv_split_append(ARGV *, const char *, const char *);

#define ARGV_END    ((char *) 0)


#endif

