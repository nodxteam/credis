#ifndef STORE_RHASH_H
#define STORE_RHASH_H

#include "platform.h"

#define RHASH_BUCKETS 64

typedef struct RHashField {
    char             *field;
    char             *value;
    struct RHashField *next;
} RHashField;

typedef struct {
    RHashField **table;
    int          count;
} RHash;

RHash      *rhash_new(void);
void        rhash_free(RHash *h);
int         rhash_set(RHash *h, const char *field, const char *value);
const char *rhash_get(RHash *h, const char *field);
int         rhash_del(RHash *h, const char *field);
int         rhash_exists(RHash *h, const char *field);

/* Returns a malloc'd array of field pointers (not copies). Count in *n. */
char      **rhash_keys(RHash *h, int *n);
char      **rhash_vals(RHash *h, int *n);

#endif
