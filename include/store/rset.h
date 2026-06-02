#ifndef STORE_RSET_H
#define STORE_RSET_H

#include "platform.h"

#define RSET_BUCKETS 64

typedef struct RSetMember {
    char              *member;
    struct RSetMember *next;
} RSetMember;

typedef struct {
    RSetMember **table;
    int          count;
} RSet;

RSet  *rset_new(void);
void   rset_free(RSet *s);
int    rset_add(RSet *s, const char *member);       /* 1=added, 0=already existed */
int    rset_rem(RSet *s, const char *member);       /* 1=removed, 0=not found */
int    rset_ismember(RSet *s, const char *member);
char **rset_members(RSet *s, int *n);               /* malloc'd array, caller frees */

#endif
