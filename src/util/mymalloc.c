/*
 *
 *
 * */

/* System libraries. */

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

/* Application-specific. */

#include "msg.h"
#include "mymalloc.h"

typedef struct MBLOCK {
    int     signature;          /* set when block is active */
    int     length;         /* user requested length */
    char    payload[1];         /* actually a bunch of bytes */
} MBLOCK;

#define SIGNATURE   0xdead
#define FILLER      0xff

#define CHECK_IN_PTR(ptr, real_ptr, len, fname) { \
        if (ptr == 0) \
        msg_panic("%s: null pointer input", fname); \
        real_ptr = (MBLOCK *) (ptr - offsetof(MBLOCK, payload[0])); \
        if (real_ptr->signature != SIGNATURE) \
        msg_panic("%s: corrupt or unallocated memory block", fname); \
        real_ptr->signature = 0; \
        if ((len = real_ptr->length) < 1) \
        msg_panic("%s: corrupt memory block length", fname); \
}

#define CHECK_OUT_PTR(ptr, real_ptr, len) { \
        real_ptr->signature = SIGNATURE; \
        real_ptr->length = len; \
        ptr = real_ptr->payload; \
}

#define SPACE_FOR(len)  (offsetof(MBLOCK, payload[0]) + len)


#ifndef NO_SHARED_EMPTY_STRINGS
static const char empty_string[] = "";
#endif

/* mymalloc - allocate memory or bust */

char *mymalloc(int len)
{
    char   *ptr;
    MBLOCK *real_ptr;

    if (len < 1)
        msg_panic("mymalloc: requested length %d", len);
    if ((real_ptr = (MBLOCK *) malloc(SPACE_FOR(len))) == 0)
        msg_fatal("mymalloc: insufficient memory: %s.",strerror(errno));
    
    CHECK_OUT_PTR(ptr, real_ptr, len);
    memset(ptr, FILLER, len);
    return (ptr);
}

/* myrealloc - reallocate memory or bust */

char *myrealloc(char *ptr, int len)
{
    MBLOCK *real_ptr;
    int     old_len;

    if (len < 1)
        msg_panic("myrealloc: requested length %d", len);
    CHECK_IN_PTR(ptr, real_ptr, old_len, "myrealloc");
    if ((real_ptr = (MBLOCK *) realloc((char *) real_ptr, SPACE_FOR(len))) == 0)
        msg_fatal("myrealloc: insufficient memory: %s", strerror(errno));
    CHECK_OUT_PTR(ptr, real_ptr, len);
    if (len > old_len)
        memset(ptr + old_len, FILLER, len - old_len);
    return (ptr);
}

/* myfree - release memory */

void myfree(char *ptr)
{
    MBLOCK *real_ptr;
    int     len;

    CHECK_IN_PTR(ptr, real_ptr, len, "myfree");
    memset((char *) real_ptr, FILLER, SPACE_FOR(len));
    free((char *) real_ptr);
}

/* mystrdup - save string to heap */
char *mystrdup(const char *str)
{
	if (str == 0)
		msg_panic("mystrdup: null pointer argument");
#ifndef NO_SHARED_EMPTY_STRINGS
	if (*str == 0)
		return ((char *) empty_string);
#endif
	return (strcpy(mymalloc(strlen(str) + 1), str));
}

/* mystrndup - save substring to heap */

char *mystrndup(const char *str, size_t len)
{
	char   *result;
	char   *cp;

	if (str == 0)
		msg_panic("mystrndup: null pointer argument");
	if (len < 0)
		msg_panic("mystrndup: requested length %ld", (long) len);
#ifndef NO_SHARED_EMPTY_STRINGS
	if (*str == 0)
		return ((char *) empty_string);
#endif
	if ((cp = memchr(str, 0, len)) != 0)
		len = cp - str;
	result = memcpy(mymalloc(len + 1), str, len);
	result[len] = 0;
	return (result);
}

/* mymemdup - copy memory */

char *mymemdup(const char *ptr, size_t len)
{
	if (ptr == 0)
		msg_panic("mymemdup: null pointer argument");
	return (memcpy(mymalloc(len), ptr, len));
}
