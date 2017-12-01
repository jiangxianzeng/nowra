#ifndef _RING_H_INCLUDED_
#define _RING_H_INCLUDED_


 /*
  *   * External interface.
  *     */
typedef struct RING RING;

struct RING {
    RING   *succ;           /* successor */
    RING   *pred;           /* predecessor */
};

extern void ring_init(RING *);
extern void ring_prepend(RING *, RING *);
extern void ring_append(RING *, RING *);
extern void ring_detach(RING *);

#define RING_SUCC(c) ((c)->succ)
#define RING_PRED(c) ((c)->pred)

#endif
