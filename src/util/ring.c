/*
 *
 *
 * */


/* System libraries. */

/* Application-specific. */

#include "ring.h"

/* ring_init - initialize ring head */

void ring_init(RING   *ring)
{
    ring->pred = ring->succ = ring;
}

/* ring_append - insert entry after ring head */

void ring_append(RING *ring,RING *entry)
{
    entry->succ = ring->succ;
    entry->pred = ring;
    ring->succ->pred = entry;
    ring->succ = entry;
}

/* ring_prepend - insert new entry before ring head */

void ring_prepend(RING *ring,RING *entry)
{
    entry->pred = ring->pred;
    entry->succ = ring;
    ring->pred->succ = entry;
    ring->pred = entry;
}

/* ring_detach - remove entry from ring */

void ring_detach(RING *entry)
{
    RING   *succ = entry->succ;
    RING   *pred = entry->pred;

    pred->succ = succ;
    succ->pred = pred;

    entry->succ = entry->pred = 0;
}
