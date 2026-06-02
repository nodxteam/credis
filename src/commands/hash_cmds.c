#include "commands/hash_cmds.h"
#include "store/rhash.h"
#include "resp/writer.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define WRONG_TYPE do { \
    resp_err(&c->io,"WRONGTYPE Operation against a key holding the wrong kind of value"); \
    mutex_unlock(&c->db->lock); return; } while(0)

static RHash *get_or_create_hash(Client *c, const char *key) {
    KVEntry *e = db_find(c->db, key);
    if (!e) { RHash *h=rhash_new(); db_set(c->db,key,RTYPE_HASH,h); return h; }
    if (e->type != RTYPE_HASH) return NULL;
    return (RHash *)e->value;
}

void cmd_hset(Client *c) {
    if ((c->argc - 2) % 2 != 0) { resp_err(&c->io,"wrong number of arguments"); return; }
    mutex_lock(&c->db->lock);
    RHash *h = get_or_create_hash(c, c->argv[1]);
    if (!h) WRONG_TYPE;
    int added = 0;
    for (int i = 2; i < c->argc; i += 2)
        added += rhash_set(h, c->argv[i], c->argv[i+1]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, added);
}

void cmd_hget(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_null_bulk(&c->io); return; }
    if (e->type != RTYPE_HASH) WRONG_TYPE;
    const char *v = rhash_get((RHash *)e->value, c->argv[2]);
    if (v) resp_bulk(&c->io, v, (int)strlen(v));
    else   resp_null_bulk(&c->io);
    mutex_unlock(&c->db->lock);
}

void cmd_hmget(Client *c) {
    resp_array_hdr(&c->io, c->argc - 2);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) {
        mutex_unlock(&c->db->lock);
        for (int i = 2; i < c->argc; i++) resp_null_bulk(&c->io);
        return;
    }
    if (e->type != RTYPE_HASH) WRONG_TYPE;
    RHash *h = (RHash *)e->value;
    for (int i = 2; i < c->argc; i++) {
        const char *v = rhash_get(h, c->argv[i]);
        v ? resp_bulk(&c->io, v, (int)strlen(v)) : resp_null_bulk(&c->io);
    }
    mutex_unlock(&c->db->lock);
}

void cmd_hgetall(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_array_hdr(&c->io, 0); return; }
    if (e->type != RTYPE_HASH) WRONG_TYPE;
    RHash *h = (RHash *)e->value;
    resp_array_hdr(&c->io, h->count * 2);
    for (int b = 0; b < RHASH_BUCKETS; b++)
        for (RHashField *f = h->table[b]; f; f = f->next) {
            resp_bulk(&c->io, f->field, (int)strlen(f->field));
            resp_bulk(&c->io, f->value, (int)strlen(f->value));
        }
    mutex_unlock(&c->db->lock);
}

void cmd_hdel(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_int(&c->io, 0); return; }
    if (e->type != RTYPE_HASH) WRONG_TYPE;
    int deleted = 0;
    for (int i = 2; i < c->argc; i++)
        deleted += rhash_del((RHash *)e->value, c->argv[i]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, deleted);
}

void cmd_hexists(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int found = 0;
    if (e && e->type == RTYPE_HASH)
        found = rhash_exists((RHash *)e->value, c->argv[2]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, found);
}

void cmd_hlen(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int len = (e && e->type == RTYPE_HASH) ? ((RHash *)e->value)->count : 0;
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, len);
}

void cmd_hkeys(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_array_hdr(&c->io, 0); return; }
    if (e->type != RTYPE_HASH) WRONG_TYPE;
    int n; char **keys = rhash_keys((RHash *)e->value, &n);
    resp_array_hdr(&c->io, n);
    for (int i = 0; i < n; i++) resp_bulk(&c->io, keys[i], (int)strlen(keys[i]));
    mutex_unlock(&c->db->lock);
    free(keys);
}

void cmd_hvals(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_array_hdr(&c->io, 0); return; }
    if (e->type != RTYPE_HASH) WRONG_TYPE;
    int n; char **vals = rhash_vals((RHash *)e->value, &n);
    resp_array_hdr(&c->io, n);
    for (int i = 0; i < n; i++) resp_bulk(&c->io, vals[i], (int)strlen(vals[i]));
    mutex_unlock(&c->db->lock);
    free(vals);
}

void cmd_hincrby(Client *c) {
    long long delta = atoll(c->argv[3]);
    mutex_lock(&c->db->lock);
    RHash *h = get_or_create_hash(c, c->argv[1]);
    if (!h) WRONG_TYPE;
    const char *cur_s = rhash_get(h, c->argv[2]);
    long long cur = cur_s ? atoll(cur_s) : 0;
    cur += delta;
    char buf[32]; snprintf(buf, sizeof(buf), "%lld", cur);
    rhash_set(h, c->argv[2], buf);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, cur);
}
