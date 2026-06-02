#ifndef PERSIST_AOF_H
#define PERSIST_AOF_H

#include "platform.h"

typedef struct {
    FILE    *fp;
    mutex_t  mutex;
    int      replaying; /* set to 1 during startup replay */
} AofLog;

AofLog *aof_open(const char *path);
/* Append a command to the AOF file as a RESP array */
void    aof_append(AofLog *aof, char **argv, int argc);
/* Replay all commands on startup. Returns number of commands replayed. */
int     aof_replay(AofLog *aof, struct DB *db);
void    aof_close(AofLog *aof);

/* Forward declaration */
struct DB;

#endif
