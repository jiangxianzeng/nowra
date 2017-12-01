/*
 * binhash.c
 *
*/
/* System libraries. */

#include <string.h>

/* Application-specific. */
#include "mymalloc.h"
#include "msg.h"
#include "binhash.h"

/* binhash_hash - hash a string */

static unsigned binhash_hash(const char *key, int len, unsigned size)
{
    unsigned long h = 0;
    unsigned long g;

    /*
     * From the "Dragon" book by Aho, Sethi and Ullman.
     */

    while (len-- > 0) {
    h = (h << 4) + *key++;
    if ((g = (h & 0xf0000000)) != 0) {
        h ^= (g >> 24);
        h ^= g;
    }
    }
    return (h % size);
}

/* binhash_link - insert element into table */

#define binhash_link(table, elm) { \
    BINHASH_INFO **h = table->data + binhash_hash(elm->key, elm->key_len, table->size);\
    elm->prev = 0; \
    if ((elm->next = *h) != 0) \
    (*h)->prev = elm; \
    *h = elm; \
    table->used++; \
}

static void binhash_size(BINHASH *table, unsigned size)
{
    BINHASH_INFO **h;

    size |= 1;
    table->data = h = (BINHASH_INFO **)mymalloc(size * sizeof(BINHASH_INFO *));
    table->size = size;
    table->used = 0;

    while (size-- > 0)
    {
        *h++ = 0;
    }
}

/* binhash_create - create initial hash table */
BINHASH *binhash_create(int size)
{
    BINHASH *table;

    table = (BINHASH *)mymalloc(sizeof(BINHASH));
    binhash_size(table, size < 13 ? 13 : size);

    return table;
}

/* binhash_grow - extend existing table */

static void binhash_grow(BINHASH *table)
{
    BINHASH_INFO *ht;
    BINHASH_INFO *next;
    unsigned old_size = table->size;
    BINHASH_INFO **h = table->data;
    BINHASH_INFO **old_entries = h;   

    binhash_size(table, 2 * old_size);

    while (old_size-- > 0) {
    for (ht = *h++; ht; ht = next) {
        next = ht->next;
        binhash_link(table, ht);
    }
    }
    myfree((char *) old_entries); 
}

/* binhash_enter - enter (key, value) pair */

BINHASH_INFO *binhash_enter(BINHASH *table, const char *key, int key_len, char *value)
{
    BINHASH_INFO *ht;

    if (table->used >= table->size)
    {
        binhash_grow(table);
    }

    ht = (BINHASH_INFO *) mymalloc(sizeof(BINHASH_INFO));
    ht->key = mymemdup(key, key_len);
    ht->key_len = key_len;
    ht->value = value;
    binhash_link(table, ht);

    return (ht); 
}

/* binhash_find - lookup value */

char *binhash_find(BINHASH *table, const char *key, int key_len)
{
    BINHASH_INFO *ht;

#define KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

    if (table != 0)
        for (ht = table->data[binhash_hash(key, key_len, table->size)]; ht; ht = ht->next)
            if (key_len == ht->key_len && KEY_EQ(key, ht->key, key_len))
                return (ht->value);
    return (0);
}

/* binhash_locate - lookup entry */

BINHASH_INFO *binhash_locate(BINHASH *table, const char *key, int key_len)
{
    BINHASH_INFO *ht;

#define KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

    if (table != 0)
        for (ht = table->data[binhash_hash(key, key_len, table->size)]; ht; ht = ht->next)
            if (key_len == ht->key_len && KEY_EQ(key, ht->key, key_len))
                return (ht);
    return (0);
}



/* binhash_delete - delete one entry */

void    binhash_delete(BINHASH *table, const char *key, int key_len, void (*free_fn) (char *))
{
    if (table != 0) 
    {
        BINHASH_INFO *ht;
        BINHASH_INFO **h = table->data + binhash_hash(key, key_len, table->size);

#define KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

        for (ht = *h; ht; ht = ht->next) {
            if (key_len == ht->key_len && KEY_EQ(key, ht->key, key_len)) {
                if (ht->next)
                    ht->next->prev = ht->prev;
                if (ht->prev)
                    ht->prev->next = ht->next;
                else
                    *h = ht->next;
                table->used--;
                myfree(ht->key);
                if (free_fn)
                    (*free_fn) (ht->value);
                myfree((char *) ht);
                return;
            }
        }
        msg_panic("binhash_delete: unknown_key: \"%s\"", key);
    }
}


/* binhash_free - destroy hash table */

void    binhash_free(BINHASH *table, void (*free_fn) (char *))
{
    if (table != 0) {
        unsigned i = table->size;
        BINHASH_INFO *ht;
        BINHASH_INFO *next;
        BINHASH_INFO **h = table->data;

        while (i-- > 0) {
            for (ht = *h++; ht; ht = next) {
                next = ht->next;
                myfree(ht->key);
                if (free_fn)
                    (*free_fn) (ht->value);
                myfree((char *) ht);
            }
        }
        myfree((char *) table->data);
        table->data = 0;
        myfree((char *) table);
    }
}

/* binhash_walk - iterate over hash table */

void    binhash_walk(BINHASH *table, void (*action) (BINHASH_INFO *, char *), char *ptr) 
{
    if (table != 0) {
        unsigned i = table->size;
        BINHASH_INFO **h = table->data;
        BINHASH_INFO *ht;

        while (i-- > 0)
            for (ht = *h++; ht; ht = ht->next)
                (*action) (ht, ptr);
    }
}


/* binhash_list - list all table members */

BINHASH_INFO **binhash_list(BINHASH *table)
{
    BINHASH_INFO **list;
    BINHASH_INFO *member;
    int     count = 0;
    int     i;

    if (table != 0) {
    list = (BINHASH_INFO **) mymalloc(sizeof(*list) * (table->used + 1));
    for (i = 0; i < table->size; i++)
        for (member = table->data[i]; member != 0; member = member->next)
        list[count++] = member;
    } else {
    list = (BINHASH_INFO **) mymalloc(sizeof(*list));
    }
    list[count] = 0;
    return (list);
}

