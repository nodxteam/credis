#ifndef COMMANDS_DISPATCHER_H
#define COMMANDS_DISPATCHER_H

#include "platform.h"
#include "net/io.h"
#include "store/db.h"
#include "persist/aof.h"

#define MAX_CMD_ARGS 128
#define RBUF_SIZE    16384

/* Per-connection client context */
typedef struct {
    IoCtx   io;
    char    client_ip[64];
    DB     *db;
    AofLog *aof;
    /* Buffered reader */
    char    rbuf[RBUF_SIZE];
    size_t  rbuf_pos;
    size_t  rbuf_len;
    /* Current parsed command */
    int     argc;
    char   *argv[MAX_CMD_ARGS];
    int     arglens[MAX_CMD_ARGS];
} Client;

typedef void (*CmdFn)(Client *c);

typedef struct {
    const char *name;
    CmdFn       fn;
    int         arity;    /* min args including cmd name; -N = at least N */
    int         is_write; /* 1 = append to AOF */
} Command;

void cmd_register(const char *name, CmdFn fn, int arity, int is_write);
void cmd_dispatch(Client *c);
void cmd_register_all(void);

/* Free argv strings allocated by the RESP parser */
void client_free_args(Client *c);

#endif /* COMMANDS_DISPATCHER_H */
