/* System library. */

#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>			/* 44bsd stdarg.h uses abort() */
#include <stdio.h>			/* sprintf() prototype */
#include <float.h>			/* range of doubles */
#include <errno.h>
#include <limits.h>			/* CHAR_BIT */

/* Application-specific. */

#include "msg.h"
#include "vbuf.h"
#include "vstring.h"
#include "vbuf_print.h"

//#define isascii(c)	((unsigned)(c)<=0177)
#define ISASCII(c)	isascii(_UCHAR_(c))
#define _UCHAR_(c)	((unsigned char)(c))
#define ISDIGIT(c)	(ISASCII(c) && isdigit((unsigned char)(c)))

#define INT_SPACE	((CHAR_BIT * sizeof(long)) / 2)
#define DBL_SPACE	((CHAR_BIT * sizeof(double)) / 2 + DBL_MAX_10_EXP)
#define PTR_SPACE	((CHAR_BIT * sizeof(char *)) / 2)

/*
 * Helper macros... Note that there is no need to check the result from
 * VSTRING_SPACE() because that always succeeds or never returns.
 */
#define VBUF_SKIP(bp)	{ \
	while ((bp)->cnt > 0 && *(bp)->ptr) \
	(bp)->ptr++, (bp)->cnt--; \
}

#define VSTRING_ADDNUM(vp, n) { \
	VSTRING_SPACE(vp, INT_SPACE); \
	sprintf(VSTRING_END(vp), "%d", n); \
	VBUF_SKIP(&vp->vbuf); \
}

#define VBUF_STRCAT(bp, s) { \
	unsigned char *_cp = (unsigned char *) (s); \
	int _ch; \
	while ((_ch = *_cp++) != 0) \
	VBUF_PUT((bp), _ch); \
}

/* vbuf_print - format string, vsprintf-like interface */

VBUF   *vbuf_print(VBUF *bp, const char *format, va_list ap)
{
	const char *myname = "vbuf_print";
	static VSTRING *fmt;		/* format specifier */
	unsigned char *cp;
	int     width;			/* width and numerical precision */
	int     prec;			/* are signed for overflow defense */
	unsigned long_flag;			/* long or plain integer */
	int     ch;
	char   *s;

	/*
	 * Assume that format strings are short.
	 */
	if (fmt == 0)
		fmt = vstring_alloc(INT_SPACE);

	/*
	 * Iterate over characters in the format string, picking up arguments
	 * when format specifiers are found.
	 */
	for (cp = (unsigned char *) format; *cp; cp++) {
		if (*cp != '%') {
			VBUF_PUT(bp, *cp);			/* ordinary character */
		} else if (cp[1] == '%') {
			VBUF_PUT(bp, *cp++);		/* %% becomes % */
		} else {

			/*
			 * Handle format specifiers one at a time, since we can only deal
			 * with arguments one at a time. Try to determine the end of the
			 * format specifier. We do not attempt to fully parse format
			 * strings, since we are ging to let sprintf() do the hard work.
			 * In regular expression notation, we recognize:
			 * %-?0?([0-9]+|\*)?\.?([0-9]+|\*)?l?[a-zA-Z]
			 * 
			 * which includes some combinations that do not make sense. Garbage
			 * in, garbage out.
			 */
			VSTRING_RESET(fmt);			/* clear format string */
			VSTRING_ADDCH(fmt, *cp++);
			if (*cp == '-')			/* left-adjusted field? */
				VSTRING_ADDCH(fmt, *cp++);
			if (*cp == '+')			/* signed field? */
				VSTRING_ADDCH(fmt, *cp++);
			if (*cp == '0')			/* zero-padded field? */
				VSTRING_ADDCH(fmt, *cp++);
			if (*cp == '*') {			/* dynamic field width */
				width = va_arg(ap, int);
				VSTRING_ADDNUM(fmt, width);
				cp++;
			} else {				/* hard-coded field width */
				for (width = 0; ch = *cp, ISDIGIT(ch); cp++) {
					width = width * 10 + ch - '0';
					VSTRING_ADDCH(fmt, ch);
				}
			}
			if (width < 0) {
				msg_warn("%s: bad width %d in %.50s", myname, width, format);
				width = 0;
			}
			if (*cp == '.')			/* width/precision separator */
				VSTRING_ADDCH(fmt, *cp++);
			if (*cp == '*') {			/* dynamic precision */
				prec = va_arg(ap, int);
				VSTRING_ADDNUM(fmt, prec);
				cp++;
			} else {				/* hard-coded precision */
				for (prec = 0; ch = *cp, ISDIGIT(ch); cp++) {
					prec = prec * 10 + ch - '0';
					VSTRING_ADDCH(fmt, ch);
				}
			}
			if (prec < 0) {
				msg_warn("%s: bad precision %d in %.50s", myname, prec, format);
				prec = 0;
			}
			if ((long_flag = (*cp == 'l')) != 0)/* long whatever */
				VSTRING_ADDCH(fmt, *cp++);
			if (*cp == 0)			/* premature end, punt */
				break;
			VSTRING_ADDCH(fmt, *cp);		/* type (checked below) */
			VSTRING_TERMINATE(fmt);		/* null terminate */

			/*
			 * Execute the format string - let sprintf() do the hard work for
			 * non-trivial cases only. For simple string conversions and for
			 * long string conversions, do a direct copy to the output
			 * buffer.
			 */
			switch (*cp) {
				case 's':				/* string-valued argument */
					s = va_arg(ap, char *);
					if (prec > 0 || (width > 0 && width > strlen(s))) {
						if (VBUF_SPACE(bp, (width > prec ? width : prec) + INT_SPACE))
							return (bp);
						sprintf((char *) bp->ptr, VSTRING_STR(fmt), s);
						VBUF_SKIP(bp);
					} else {
						VBUF_STRCAT(bp, s);
					}
					break;
				case 'c':				/* integral-valued argument */
				case 'd':
				case 'u':
				case 'o':
				case 'x':
				case 'X':
					if (VBUF_SPACE(bp, (width > prec ? width : prec) + INT_SPACE))
						return (bp);
					if (long_flag)
						sprintf((char *) bp->ptr, VSTRING_STR(fmt), va_arg(ap, long));
					else
						sprintf((char *) bp->ptr, VSTRING_STR(fmt), va_arg(ap, int));
					VBUF_SKIP(bp);
					break;
				case 'e':				/* float-valued argument */
				case 'f':
				case 'g':
					if (VBUF_SPACE(bp, (width > prec ? width : prec) + DBL_SPACE))
						return (bp);
					sprintf((char *) bp->ptr, VSTRING_STR(fmt), va_arg(ap, double));
					VBUF_SKIP(bp);
					break;
				case 'm':
					VBUF_STRCAT(bp, strerror(errno));
					break;
				case 'p':
					if (VBUF_SPACE(bp, (width > prec ? width : prec) + PTR_SPACE))
						return (bp);
					sprintf((char *) bp->ptr, VSTRING_STR(fmt), va_arg(ap, char *));
					VBUF_SKIP(bp);
					break;
				default:				/* anything else is bad */
					msg_panic("vbuf_print: unknown format type: %c", *cp);
					/* NOTREACHED */
					break;
			}
		}
	}
	return (bp);
}
