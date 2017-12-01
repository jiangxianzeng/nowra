#ifndef _MALLOC_H_INCLUDED_
#define _MALLOC_H_INCLUDED_

/*
 * External interface.
 */
extern char *mymalloc(int);
extern char *myrealloc(char *, int);
extern void myfree(char *);

extern char *mystrdup(const char *);
extern char *mystrndup(const char *, size_t);
extern char *mymemdup(const char *, size_t);

#endif
