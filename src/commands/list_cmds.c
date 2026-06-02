#include "commands/list_cmds.h"
#include "store/rlist.h"
#include "resp/writer.h"
#include <string.h>
#include <stdlib.h>

#define WRONG_TYPE do { \
    resp_err(&c->io,"WRONGTYPE Operation against a key holding the wrong kind of value"); \
    mutex_unlock(&c->db->lock); return; } while(0)

static RList *get_or_create_list(Client *c, const char *key) {
    KVEntry *e = db_find(c->db, key);
    if (!e) {
        RList *l = rlist_new();
        db_set(c->db, key, RTYPE_LIST, l);
        return l;
    }
    if (e->type != RTYPE_LIST) return NULL;
    return (RList *)e->value;
}

static void push_multi(Client *c, int left) {
    mutex_lock(&c->db->lock);
    RList *l = get_or_create_list(c, c->argv[1]);
    if (!l) WRONG_TYPE;
    for (int i = 2; i < c->argc; i++)
        left ? rlist_lpush(l, c->argv[i]) : rlist_rpush(l, c->argv[i]);
    long len = l->len;
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, len);
}

void cmd_lpush(Client *c) { push_multi(c, 1); }
void cmd_rpush(Client *c) { push_multi(c, 0); }

static void pop_one(Client *c, int left) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_null_bulk(&c->io); return; }
    if (e->type != RTYPE_LIST) WRONG_TYPE;
    RList *l = (RList *)e->value;
    char *val = left ? rlist_lpop(l) : rlist_rpop(l);
    if (l->len == 0) db_del(c->db, c->argv[1]);
    mutex_unlock(&c->db->lock);
    if (val) { resp_bulk(&c->io, val, (int)strlen(val)); free(val); }
    else resp_null_bulk(&c->io);
}

void cmd_lpop(Client *c) { pop_one(c, 1); }
void cmd_rpop(Client *c) { pop_one(c, 0); }

void cmd_lrange(Client *c) {
    long start = atol(c->argv[2]);
    long stop  = atol(c->argv[3]);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_array_hdr(&c->io, 0); return; }
    if (e->type != RTYPE_LIST) WRONG_TYPE;
    RList *l = (RList *)e->value;
    long len = l->len;
    if (start < 0) start = len + start;
    if (stop  < 0) stop  = len + stop;
    if (start < 0) start = 0;
    if (stop >= len) stop = len - 1;
    if (start > stop) { mutex_unlock(&c->db->lock); resp_array_hdr(&c->io, 0); return; }
    int count = (int)(stop - start + 1);
    resp_array_hdr(&c->io, count);
    for (long i = start; i <= stop; i++) {
        RListNode *n = rlist_index(l, i);
        if (n) resp_bulk(&c->io, n->val, (int)strlen(n->val));
        else   resp_null_bulk(&c->io);
    }
    mutex_unlock(&c->db->lock);
}

void cmd_llen(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_int(&c->io, 0); return; }
    if (e->type != RTYPE_LIST) WRONG_TYPE;
    long len = ((RList *)e->value)->len;
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, len);
}

void cmd_lindex(Client *c) {
    long idx = atol(c->argv[2]);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_null_bulk(&c->io); return; }
    if (e->type != RTYPE_LIST) WRONG_TYPE;
    RListNode *n = rlist_index((RList *)e->value, idx);
    if (n) resp_bulk(&c->io, n->val, (int)strlen(n->val));
    else   resp_null_bulk(&c->io);
    mutex_unlock(&c->db->lock);
}

void cmd_lset(Client *c) {
    long idx = atol(c->argv[2]);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_err(&c->io, "no such key"); return; }
    if (e->type != RTYPE_LIST) WRONG_TYPE;
    RListNode *n = rlist_index((RList *)e->value, idx);
    if (!n) { mutex_unlock(&c->db->lock); resp_err(&c->io, "index out of range"); return; }
    free(n->val); n->val = strdup(c->argv[3]);
    mutex_unlock(&c->db->lock);
    resp_ok(&c->io);
}

void cmd_lrem(Client *c) {
    long count = atol(c->argv[2]);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_int(&c->io, 0); return; }
    if (e->type != RTYPE_LIST) WRONG_TYPE;
    long removed = rlist_lrem((RList *)e->value, count, c->argv[3]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, removed);
}

void cmd_ltrim(Client *c) {
    long start = atol(c->argv[2]);
    long stop  = atol(c->argv[3]);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (e && e->type != RTYPE_LIST) WRONG_TYPE;
    if (e) rlist_ltrim((RList *)e->value, start, stop);
    mutex_unlock(&c->db->lock);
    resp_ok(&c->io);
}
