#include "store/db.h"
#include "store/rlist.h"
#include "store/rhash.h"
#include "store/rset.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

static unsigned int hash_key(const char *s) {
    unsigned int h = 5381;
    for (; *s; s++) h = ((h << 5) + h) ^ (unsigned char)*s;
    return h % DB_BUCKETS;
}

DB *db_create(void) {
    DB *db = (DB *)calloc(1, sizeof(DB));
    db->table = (KVEntry **)calloc(DB_BUCKETS, sizeof(KVEntry *));
    mutex_init(&db->lock);
    return db;
}

const char *rtype_name(RType t) {
    switch (t) {
        case RTYPE_STRING: return "string";
        case RTYPE_LIST:   return "list";
        case RTYPE_HASH:   return "hash";
        case RTYPE_SET:    return "set";
        default:           return "none";
    }
}

void db_free_value(RType type, void *value) {
    if (!value) return;
    switch (type) {
        case RTYPE_STRING: free(value); break;
        case RTYPE_LIST:   rlist_free((RList *)value); break;
        case RTYPE_HASH:   rhash_free((RHash *)value); break;
        case RTYPE_SET:    rset_free((RSet *)value); break;
    }
}

/* Caller must hold lock */
KVEntry *db_find(DB *db, const char *key) {
    unsigned int h = hash_key(key);
    KVEntry *e = db->table[h];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            /* Lazy expiry */
            if (e->expire_at && time(NULL) >= e->expire_at) {
                /* Expired — treat as not found; purge on next cleanup */
                return NULL;
            }
            return e;
        }
        e = e->next;
    }
    return NULL;
}

KVEntry *db_set(DB *db, const char *key, RType type, void *value) {
    unsigned int h = hash_key(key);
    KVEntry *e = db->table[h];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            db_free_value(e->type, e->value);
            e->type      = type;
            e->value     = value;
            e->expire_at = 0;
            return e;
        }
        e = e->next;
    }
    KVEntry *ne = (KVEntry *)calloc(1, sizeof(KVEntry));
    ne->key   = strdup(key);
    ne->type  = type;
    ne->value = value;
    ne->next  = db->table[h];
    db->table[h] = ne;
    db->count++;
    return ne;
}

int db_del(DB *db, const char *key) {
    unsigned int h = hash_key(key);
    KVEntry **pp = &db->table[h];
    while (*pp) {
        if (strcmp((*pp)->key, key) == 0) {
            KVEntry *del = *pp;
            *pp = del->next;
            db_free_value(del->type, del->value);
            free(del->key);
            free(del);
            db->count--;
            return 1;
        }
        pp = &(*pp)->next;
    }
    return 0;
}

void db_flush(DB *db) {
    for (int i = 0; i < DB_BUCKETS; i++) {
        KVEntry *e = db->table[i];
        while (e) {
            KVEntry *nx = e->next;
            db_free_value(e->type, e->value);
            free(e->key);
            free(e);
            e = nx;
        }
        db->table[i] = NULL;
    }
    db->count = 0;
}

void db_purge_expired(DB *db) {
    time_t now = time(NULL);
    mutex_lock(&db->lock);
    for (int i = 0; i < DB_BUCKETS; i++) {
        KVEntry **pp = &db->table[i];
        while (*pp) {
            KVEntry *e = *pp;
            if (e->expire_at && now >= e->expire_at) {
                *pp = e->next;
                db_free_value(e->type, e->value);
                free(e->key);
                free(e);
                db->count--;
            } else {
                pp = &(*pp)->next;
            }
        }
    }
    mutex_unlock(&db->lock);
}

void db_destroy(DB *db) {
    db_flush(db);
    free(db->table);
    mutex_destroy(&db->lock);
    free(db);
}
