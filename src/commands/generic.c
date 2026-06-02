#include "commands/generic.h"
#include "resp/writer.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fnmatch.h>

/* Simple glob match: * and ? wildcards */
static int glob_match(const char *pat, const char *str) {
    while (*pat) {
        if (*pat == '*') {
            while (*pat == '*') pat++;
            if (!*pat) return 1;
            while (*str)
                if (glob_match(pat, str++)) return 1;
            return 0;
        } else if (*pat == '?') {
            if (!*str) return 0;
            pat++; str++;
        } else {
            if (*pat != *str) return 0;
            pat++; str++;
        }
    }
    return *str == '\0';
}

void cmd_ping(Client *c) {
    if (c->argc == 2)
        resp_bulk(&c->io, c->argv[1], c->arglens[1]);
    else
        resp_status(&c->io, "PONG");
}

void cmd_quit(Client *c) {
    resp_ok(&c->io);
    io_close(&c->io);
}

void cmd_del(Client *c) {
    int deleted = 0;
    mutex_lock(&c->db->lock);
    for (int i = 1; i < c->argc; i++)
        deleted += db_del(c->db, c->argv[i]);
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, deleted);
}

void cmd_exists(Client *c) {
    int count = 0;
    mutex_lock(&c->db->lock);
    for (int i = 1; i < c->argc; i++)
        if (db_find(c->db, c->argv[i])) count++;
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, count);
}

void cmd_type(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    const char *t = e ? rtype_name(e->type) : "none";
    mutex_unlock(&c->db->lock);
    resp_status(&c->io, t);
}

void cmd_expire(Client *c) {
    long secs = atol(c->argv[2]);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int found = 0;
    if (e) { e->expire_at = time(NULL) + secs; found = 1; }
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, found);
}

void cmd_expireat(Client *c) {
    time_t ts = (time_t)atoll(c->argv[2]);
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int found = 0;
    if (e) { e->expire_at = ts; found = 1; }
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, found);
}

void cmd_ttl(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    long long ttl;
    if (!e)                 ttl = -2;
    else if (!e->expire_at) ttl = -1;
    else {
        ttl = (long long)(e->expire_at - time(NULL));
        if (ttl < 0) ttl = -2;
    }
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, ttl);
}

void cmd_persist(Client *c) {
    mutex_lock(&c->db->lock);
    KVEntry *e = db_find(c->db, c->argv[1]);
    int found = 0;
    if (e && e->expire_at) { e->expire_at = 0; found = 1; }
    mutex_unlock(&c->db->lock);
    resp_int(&c->io, found);
}

void cmd_keys(Client *c) {
    const char *pat = c->argv[1];
    /* Collect matching keys */
    char **matches = (char **)malloc((size_t)c->db->count * sizeof(char *));
    int   nmatch   = 0;

    mutex_lock(&c->db->lock);
    time_t now = time(NULL);
    for (int i = 0; i < DB_BUCKETS; i++) {
        KVEntry *e = c->db->table[i];
        while (e) {
            if ((!e->expire_at || now < e->expire_at) && glob_match(pat, e->key))
                matches[nmatch++] = e->key;
            e = e->next;
        }
    }
    resp_array_hdr(&c->io, nmatch);
    for (int i = 0; i < nmatch; i++)
        resp_bulk(&c->io, matches[i], (int)strlen(matches[i]));
    mutex_unlock(&c->db->lock);

    free(matches);
}

void cmd_dbsize(Client *c) {
    mutex_lock(&c->db->lock);
    resp_int(&c->io, c->db->count);
    mutex_unlock(&c->db->lock);
}

void cmd_flushdb(Client *c) {
    mutex_lock(&c->db->lock);
    db_flush(c->db);
    mutex_unlock(&c->db->lock);
    resp_ok(&c->io);
}
