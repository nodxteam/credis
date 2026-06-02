#include "commands/set_cmds.h"
#include "store/rset.h"
#include "resp/writer.h"
#include <stdlib.h>
#include <string.h>

#define WRONG_TYPE do { \
    resp_err(&c->io,"WRONGTYPE Operation against a key holding the wrong kind of value"); \
    mutex_unlock(&c->db->lock); return; } while(0)

static RSet *get_or_create_set(Client *c, const char *key) {
    KVEntry *e = db_find(c->db, key);
    if (!e) { RSet *s=rset_new(); db_set(c->db,key,RTYPE_SET,s); return s; }
    if (e->type != RTYPE_SET) return NULL;
    return (RSet *)e->value;
}

void cmd_sadd(Client *c) {
    mutex_lock(&c->db->lock);
    RSet *s = get_or_create_set(c, c->argv[1]);
    if (!s) WRONG_TYPE;
    int added = 0;
    for (int i = 2; i < c->argc; i++) added += rset_add(s, c->argv[i]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, added);
}

void cmd_srem(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_int(&c->io, 0); return; }
    if (e->type != RTYPE_SET) WRONG_TYPE;
    int removed = 0;
    for (int i = 2; i < c->argc; i++) removed += rset_rem((RSet *)e->value, c->argv[i]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, removed);
}

void cmd_smembers(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_array_hdr(&c->io, 0); return; }
    if (e->type != RTYPE_SET) WRONG_TYPE;
    int n; char **members = rset_members((RSet *)e->value, &n);
    resp_array_hdr(&c->io, n);
    for (int i = 0; i < n; i++) resp_bulk(&c->io, members[i], (int)strlen(members[i]));
    mutex_unlock(&c->db->lock);
    free(members);
}

void cmd_sismember(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int found = 0;
    if (e && e->type == RTYPE_SET)
        found = rset_ismember((RSet *)e->value, c->argv[2]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, found);
}

void cmd_scard(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int n = (e && e->type == RTYPE_SET) ? ((RSet *)e->value)->count : 0;
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, n);
}

/* Helper: build a new set from union of all arg keys */
static RSet *multi_set_op(Client *c, int start_key,
                            int do_union, int do_inter, int do_diff) {
    RSet *result = rset_new();
    int first = 1;
    for (int i = start_key; i < c->argc; i++) {
        KVEntry *e = db_find(c->db, c->argv[i]);
        if (!e || e->type != RTYPE_SET) continue;
        RSet *s = (RSet *)e->value;
        if (do_union) {
            int n; char **m = rset_members(s, &n);
            for (int j = 0; j < n; j++) rset_add(result, m[j]);
            free(m);
        } else if (do_inter) {
            if (first) {
                int n; char **m = rset_members(s, &n);
                for (int j = 0; j < n; j++) rset_add(result, m[j]);
                free(m); first = 0;
            } else {
                int n; char **m = rset_members(result, &n);
                for (int j = 0; j < n; j++)
                    if (!rset_ismember(s, m[j])) rset_rem(result, m[j]);
                free(m);
            }
        } else if (do_diff) {
            if (first) {
                int n; char **m = rset_members(s, &n);
                for (int j = 0; j < n; j++) rset_add(result, m[j]);
                free(m); first = 0;
            } else {
                int n; char **m = rset_members(s, &n);
                for (int j = 0; j < n; j++) rset_rem(result, m[j]);
                free(m);
            }
        }
    }
    return result;
}

static void set_op_reply(Client *c, RSet *result) {
    int n; char **m = rset_members(result, &n);
    resp_array_hdr(&c->io, n);
    for (int i = 0; i < n; i++) resp_bulk(&c->io, m[i], (int)strlen(m[i]));
    free(m);
    rset_free(result);
}

void cmd_sunion(Client *c) {
    mutex_lock(&c->db->lock);
    RSet *r = multi_set_op(c, 1, 1, 0, 0);
    mutex_unlock(&c->db->lock);
    set_op_reply(c, r);
}

void cmd_sinter(Client *c) {
    mutex_lock(&c->db->lock);
    RSet *r = multi_set_op(c, 1, 0, 1, 0);
    mutex_unlock(&c->db->lock);
    set_op_reply(c, r);
}

void cmd_sdiff(Client *c) {
    mutex_lock(&c->db->lock);
    RSet *r = multi_set_op(c, 1, 0, 0, 1);
    mutex_unlock(&c->db->lock);
    set_op_reply(c, r);
}
