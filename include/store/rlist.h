#ifndef STORE_RLIST_H
#define STORE_RLIST_H

#include "platform.h"

typedef struct RListNode {
    char             *val;
    struct RListNode *prev;
    struct RListNode *next;
} RListNode;

typedef struct {
    RListNode *head;
    RListNode *tail;
    long       len;
} RList;

RList     *rlist_new(void);
void       rlist_free(RList *l);
void       rlist_lpush(RList *l, const char *val);
void       rlist_rpush(RList *l, const char *val);
char      *rlist_lpop(RList *l);   /* caller must free */
char      *rlist_rpop(RList *l);   /* caller must free */
/* Returns node at 0-based index (negative = from tail). NULL if OOB. */
RListNode *rlist_index(RList *l, long idx);
long       rlist_lrem(RList *l, long count, const char *val);
void       rlist_ltrim(RList *l, long start, long stop);

#endif
