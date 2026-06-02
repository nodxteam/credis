#ifndef STORE_DB_H
#define STORE_DB_H

#include "platform.h"

#define DB_BUCKETS 4096

typedef enum {
    RTYPE_STRING,
    RTYPE_LIST,
    RTYPE_HASH,
    RTYPE_SET
} RType;

typedef struct KVEntry {
    char          *key;
    RType          type;
    void          *value;
    time_t         expire_at;  /* 0 = no expiry */
    struct KVEntry *next;
} KVEntry;

typedef struct {
    KVEntry **table;
    int       count;
    mutex_t   lock;
} DB;

DB       *db_create(void);
void      db_destroy(DB *db);

/* Returns entry if key exists and is not expired, NULL otherwise.
   Caller must hold db->lock. */
KVEntry  *db_find(DB *db, const char *key);

/* Create or overwrite an entry. Takes ownership of value.
   Caller must hold db->lock. */
KVEntry  *db_set(DB *db, const char *key, RType type, void *value);

/* Delete a key. Returns 1 if deleted, 0 if not found.
   Caller must hold db->lock. */
int       db_del(DB *db, const char *key);

/* Delete all keys. Caller must hold db->lock. */
void      db_flush(DB *db);

/* Free the value pointed to by an entry based on its type. */
void      db_free_value(RType type, void *value);

/* Active expiry: scan and delete expired keys. Thread-safe. */
void      db_purge_expired(DB *db);

const char *rtype_name(RType t);

#endif /* STORE_DB_H */
