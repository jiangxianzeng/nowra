/*
 *
 * */

/* System libraries. */

#include <string.h>
#include <stdarg.h>
#include <limits.h>

/* Application-specific. */

#include "mymalloc.h"
#include "stringop.h"

/* split_at - break string at first delimiter, return remainder */

char *split_at(char *string, int delimiter)
{
	char   *cp;

	if ((cp = strchr(string, delimiter)) != 0)
		*cp++ = 0;
	return (cp);
}

/* split_at_right - break string at last delimiter, return remainder */

char *split_at_right(char *string, int delimiter)
{
	char   *cp;

	if ((cp = strrchr(string, delimiter)) != 0)
		*cp++ = 0;
	return (cp);
}

/* mystrtok - safe tokenizer */

char *mystrtok(char **src, const char *sep)
{
	char *start = *src;
	char *end;

	/*
	 * Skip over leading delimiters.
	 */
	start += strspn(start, sep);
	if (*start == 0) {
		*src = start;
		return (0);
	}

	/*
	 * Separate off one token.
	 */
	end = start + strcspn(start, sep);
	if (*end != 0)
		*end++ = 0;
	*src = end;
	return (start);
}

char *host_port(const char *addr, char **hostp, char **portp)
{
	char   *buf;

	buf = mystrdup(addr);
	if ((*portp = split_at_right(buf, ':')) != 0) 
	{
		*hostp = buf;
	} 
	else 
	{
		*portp = buf;
		*hostp = "";
	}
	return (buf);	
}

/* concatenate - concatenate null-terminated list of strings */

char *concatenate(const char *arg0,...)
{
	char   *result;
	va_list ap;
	int     len;
	char   *arg;

	/*
	 * Compute the length of the resulting string.
	 */

	va_start(ap, arg0);
	len = strlen(arg0);
	while ((arg = va_arg(ap, char *)) != 0)
		len += strlen(arg);
	va_end(ap);

	/*
	 * Build the resulting string. Don't care about wasting a CPU cycle.
	 */
	result = mymalloc(len + 1);
	va_start(ap, arg0);
	strcpy(result, arg0);
	while ((arg = va_arg(ap, char *)) != 0)
		strcat(result, arg);
	va_end(ap);
	return (result);
}


/* basename - skip directory prefix */

char *basename(const char *path)
{
	char   *result;

	if ((result = strrchr(path, '/')) == 0)
		result = (char *) path;
	else
		result += 1;
	return (result);
}

/* Convert a string into a long long. Returns 1 if the string could be parsed
 *  * into a (non-overflowing) long long, 0 otherwise. The value will be set to
 *   * the parsed value when appropriate. */
int string2ll(const char *s, size_t slen, long long *value) {
    const char *p = s;
    size_t plen = 0;
    int negative = 0;
    unsigned long long v;

    if (plen == slen)
        return 0;

    /* Special case: first and only digit is 0. */
    if (slen == 1 && p[0] == '0') {
        if (value != NULL) *value = 0;
        return 1;
    }

    if (p[0] == '-') {
        negative = 1;
        p++; plen++;

        /* Abort on only a negative sign. */
        if (plen == slen)
            return 0;
    }

    /* First digit should be 1-9, otherwise the string should just be 0. */
    if (p[0] >= '1' && p[0] <= '9') {
        v = p[0]-'0';
        p++; plen++;
    } else if (p[0] == '0' && slen == 1) {
        *value = 0;
        return 1;
    } else {
        return 0;
    }

    while (plen < slen && p[0] >= '0' && p[0] <= '9') {
        if (v > (ULLONG_MAX / 10)) /* Overflow. */
            return 0;
        v *= 10;

        if (v > (ULLONG_MAX - (p[0]-'0'))) /* Overflow. */
            return 0;
        v += p[0]-'0';

        p++; plen++;
    }

    /* Return if not all bytes were used. */
    if (plen < slen)
        return 0;

    if (negative) {
        if (v > ((unsigned long long)(-(LLONG_MIN+1))+1)) /* Overflow. */
            return 0;
        if (value != NULL) *value = -v;
    } else {
        if (v > LLONG_MAX) /* Overflow. */
            return 0;
        if (value != NULL) *value = v;
    }
    return 1;
}

