#include "commands/string_cmds.h"
#include "resp/writer.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#define TYPE_CHECK(e, t) do { if((e)->type != (t)) { \
    resp_err(&c->io,"WRONGTYPE Operation against a key holding the wrong kind of value"); \
    mutex_unlock(&c->db->lock); return; } } while(0)

/* Get string value as long long. Returns 0 on success. */
static int str_to_ll(const char *s, long long *out) {
    char *end;
    errno = 0;
    *out = strtoll(s, &end, 10);
    return (errno || *end != '\0') ? -1 : 0;
}

void cmd_set(Client *c) {
    /* SET key value [EX seconds] [NX|XX] */
    const char *val = c->argv[2];
    long long ex_secs = 0;
    int nx = 0, xx = 0;

    for (int i = 3; i < c->argc; i++) {
        if (strcasecmp(c->argv[i], "EX") == 0 && i+1 < c->argc)
            ex_secs = atoll(c->argv[++i]);
        else if (strcasecmp(c->argv[i], "NX") == 0) nx = 1;
        else if (strcasecmp(c->argv[i], "XX") == 0) xx = 1;
    }

    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (nx && e) { mutex_unlock(&c->db->lock); resp_null_bulk(&c->io); return; }
    if (xx && !e) { mutex_unlock(&c->db->lock); resp_null_bulk(&c->io); return; }

    e = db_set(c->db, c->argv[1], RTYPE_STRING, strdup(val));
    if (ex_secs > 0) e->expire_at = time(NULL) + ex_secs;
    mutex_unlock(&c->db->lock);
    resp_ok(&c->io);
}

void cmd_get(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (!e) { mutex_unlock(&c->db->lock); resp_null_bulk(&c->io); return; }
    if (e->type != RTYPE_STRING) {
        mutex_unlock(&c->db->lock);
        resp_err(&c->io,"WRONGTYPE Operation against a key holding the wrong kind of value");
        return;
    }
    const char *s = (const char *)e->value;
    resp_bulk(&c->io, s, (int)strlen(s));
    mutex_unlock(&c->db->lock);
}

void cmd_getset(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    if (e && e->type != RTYPE_STRING) {
        mutex_unlock(&c->db->lock);
        resp_err(&c->io,"WRONGTYPE Operation against a key holding the wrong kind of value");
        return;
    }
    if (e) resp_bulk(&c->io, (char *)e->value, (int)strlen((char *)e->value));
    else   resp_null_bulk(&c->io);
    db_set(c->db, c->argv[1], RTYPE_STRING, strdup(c->argv[2]));
    mutex_unlock(&c->db->lock);
}

void cmd_setnx(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int set = 0;
    if (!e) { db_set(c->db, c->argv[1], RTYPE_STRING, strdup(c->argv[2])); set = 1; }
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, set);
}

void cmd_setex(Client *c) {
    long long secs = atoll(c->argv[2]);
    if (secs <= 0) { resp_err(&c->io, "invalid expire time"); return; }
    mutex_lock(&c->db->lock);
    KVEntry *e = db_set(c->db, c->argv[1], RTYPE_STRING, strdup(c->argv[3]));
    e->expire_at = time(NULL) + secs;
    mutex_unlock(&c->db->lock);
    resp_ok(&c->io);
}

void cmd_mset(Client *c) {
    if ((c->argc - 1) % 2 != 0) { resp_err(&c->io, "wrong number of arguments"); return; }
    mutex_lock(&c->db->lock);
    for (int i = 1; i < c->argc; i += 2)
        db_set(c->db, c->argv[i], RTYPE_STRING, strdup(c->argv[i+1]));
    mutex_unlock(&c->db->lock);
    resp_ok(&c->io);
}

void cmd_mget(Client *c) {
    resp_array_hdr(&c->io, c->argc - 1);
    mutex_lock(&c->db->lock);
    for (int i = 1; i < c->argc; i++) {
        KVEntry *e = db_find(c->db, c->argv[i]);
        if (!e || e->type != RTYPE_STRING) resp_null_bulk(&c->io);
        else resp_bulk(&c->io, (char *)e->value, (int)strlen((char *)e->value));
    }
    mutex_unlock(&c->db->lock);
}

static void incr_by(Client *c, long long delta) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    long long cur = 0;
    if (e) {
        TYPE_CHECK(e, RTYPE_STRING);
        if (str_to_ll((char *)e->value, &cur) < 0) {
            mutex_unlock(&c->db->lock);
            resp_err(&c->io, "value is not an integer or out of range"); return;
        }
    }
    cur += delta;
    char buf[32]; snprintf(buf, sizeof(buf), "%lld", cur);
    db_set(c->db, c->argv[1], RTYPE_STRING, strdup(buf));
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, cur);
}

void cmd_incr(Client *c)   { incr_by(c, 1); }
void cmd_decr(Client *c)   { incr_by(c, -1); }
void cmd_incrby(Client *c) { incr_by(c,  atoll(c->argv[2])); }
void cmd_decrby(Client *c) { incr_by(c, -atoll(c->argv[2])); }

void cmd_append(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    size_t newlen;
    char  *newval;
    if (!e) {
        newval = strdup(c->argv[2]);
        newlen = strlen(newval);
    } else {
        TYPE_CHECK(e, RTYPE_STRING);
        size_t ol = strlen((char *)e->value);
        size_t al = strlen(c->argv[2]);
        newval = (char *)malloc(ol + al + 1);
        memcpy(newval, e->value, ol);
        memcpy(newval + ol, c->argv[2], al + 1);
        newlen = ol + al;
    }
    db_set(c->db, c->argv[1], RTYPE_STRING, newval);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, (long long)newlen);
}

void cmd_strlen(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    long long len = 0;
    if (e) {
        if (e->type != RTYPE_STRING) {
            mutex_unlock(&c->db->lock);
            resp_err(&c->io,"WRONGTYPE Operation against a key holding the wrong kind of value");
            return;
        }
        len = (long long)strlen((char *)e->value);
    }
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, len);
}
