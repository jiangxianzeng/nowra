#ifndef _VBUF_PRINT_H_INCLUDED_
#define _VBUF_PRINT_H_INCLUDED_

/*    
 * System library.
 */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include "vbuf.h"

extern VBUF *vbuf_print(VBUF *, const char *, va_list);

#endif
