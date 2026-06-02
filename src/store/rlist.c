#include "store/rlist.h"
#include <stdlib.h>
#include <string.h>

RList *rlist_new(void) { return (RList *)calloc(1, sizeof(RList)); }

void rlist_free(RList *l) {
    RListNode *n = l->head;
    while (n) { RListNode *nx=n->next; free(n->val); free(n); n=nx; }
    free(l);
}

static RListNode *new_node(const char *val) {
    RListNode *n = (RListNode *)calloc(1, sizeof(RListNode));
    n->val = strdup(val);
    return n;
}

void rlist_lpush(RList *l, const char *val) {
    RListNode *n = new_node(val);
    n->next = l->head;
    if (l->head) l->head->prev = n;
    l->head = n;
    if (!l->tail) l->tail = n;
    l->len++;
}

void rlist_rpush(RList *l, const char *val) {
    RListNode *n = new_node(val);
    n->prev = l->tail;
    if (l->tail) l->tail->next = n;
    l->tail = n;
    if (!l->head) l->head = n;
    l->len++;
}

char *rlist_lpop(RList *l) {
    if (!l->head) return NULL;
    RListNode *n = l->head;
    l->head = n->next;
    if (l->head) l->head->prev = NULL;
    else l->tail = NULL;
    l->len--;
    char *val = n->val; free(n); return val;
}

char *rlist_rpop(RList *l) {
    if (!l->tail) return NULL;
    RListNode *n = l->tail;
    l->tail = n->prev;
    if (l->tail) l->tail->next = NULL;
    else l->head = NULL;
    l->len--;
    char *val = n->val; free(n); return val;
}

RListNode *rlist_index(RList *l, long idx) {
    if (idx < 0) idx = l->len + idx;
    if (idx < 0 || idx >= l->len) return NULL;
    RListNode *n;
    if (idx <= l->len / 2) {
        n = l->head;
        for (long i = 0; i < idx; i++) n = n->next;
    } else {
        n = l->tail;
        for (long i = l->len - 1; i > idx; i--) n = n->prev;
    }
    return n;
}

long rlist_lrem(RList *l, long count, const char *val) {
    int from_head = (count >= 0);
    long abs_count = count < 0 ? -count : count;
    long removed = 0;
    RListNode *n = from_head ? l->head : l->tail;
    while (n && (abs_count == 0 || removed < abs_count)) {
        RListNode *next = from_head ? n->next : n->prev;
        if (strcmp(n->val, val) == 0) {
            if (n->prev) n->prev->next = n->next; else l->head = n->next;
            if (n->next) n->next->prev = n->prev; else l->tail = n->prev;
            free(n->val); free(n);
            l->len--; removed++;
        }
        n = next;
    }
    return removed;
}

void rlist_ltrim(RList *l, long start, long stop) {
    long len = l->len;
    if (start < 0) start = len + start;
    if (stop  < 0) stop  = len + stop;
    if (start < 0) start = 0;
    if (stop >= len) stop = len - 1;
    /* Remove nodes outside [start, stop] */
    if (start > stop) { rlist_free(l); return; }
    /* Trim from head */
    for (long i = 0; i < start && l->head; i++) { char *v = rlist_lpop(l); free(v); }
    /* Trim from tail */
    long new_len = stop - start + 1;
    while (l->len > new_len && l->tail) { char *v = rlist_rpop(l); free(v); }
}
