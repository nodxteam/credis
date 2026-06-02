#include "commands/dispatcher.h"
#include "commands/generic.h"
#include "commands/string_cmds.h"
#include "commands/list_cmds.h"
#include "commands/hash_cmds.h"
#include "commands/set_cmds.h"
#include "resp/writer.h"
#include "persist/aof.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CMDS 128

static Command cmd_table[MAX_CMDS];
static int     cmd_count = 0;

void cmd_register(const char *name, CmdFn fn, int arity, int is_write) {
    if (cmd_count >= MAX_CMDS) return;
    cmd_table[cmd_count].name     = name;
    cmd_table[cmd_count].fn       = fn;
    cmd_table[cmd_count].arity    = arity;
    cmd_table[cmd_count].is_write = is_write;
    cmd_count++;
}

static const Command *cmd_lookup(const char *name) {
    for (int i = 0; i < cmd_count; i++)
        if (strcasecmp(cmd_table[i].name, name) == 0)
            return &cmd_table[i];
    return NULL;
}

void cmd_dispatch(Client *c) {
    if (c->argc == 0) return;

    const Command *cmd = cmd_lookup(c->argv[0]);
    if (!cmd) {
        resp_errf(&c->io, "unknown command '%s'", c->argv[0]);
        return;
    }

    /* Arity check: positive = exact, negative = at least |n| */
    int ok;
    if (cmd->arity > 0) ok = (c->argc == cmd->arity);
    else                 ok = (c->argc >= -cmd->arity);
    if (!ok) {
        resp_errf(&c->io, "wrong number of arguments for '%s'", c->argv[0]);
        return;
    }

    cmd->fn(c);

    /* Persist write commands to AOF */
    if (cmd->is_write && c->aof && !c->aof->replaying)
        aof_append(c->aof, c->argv, c->argc);
}

void cmd_register_all(void) {
    /* Generic */
    cmd_register("PING",    cmd_ping,    -1, 0);
    cmd_register("QUIT",    cmd_quit,     1, 0);
    cmd_register("DEL",     cmd_del,     -2, 1);
    cmd_register("EXISTS",  cmd_exists,  -2, 0);
    cmd_register("TYPE",    cmd_type,     2, 0);
    cmd_register("EXPIRE",  cmd_expire,   3, 1);
    cmd_register("EXPIREAT",cmd_expireat, 3, 1);
    cmd_register("TTL",     cmd_ttl,      2, 0);
    cmd_register("PERSIST", cmd_persist,  2, 1);
    cmd_register("KEYS",    cmd_keys,     2, 0);
    cmd_register("DBSIZE",  cmd_dbsize,   1, 0);
    cmd_register("FLUSHDB", cmd_flushdb,  1, 1);
    /* String */
    cmd_register("SET",     cmd_set,     -3, 1);
    cmd_register("GET",     cmd_get,      2, 0);
    cmd_register("GETSET",  cmd_getset,   3, 1);
    cmd_register("SETNX",   cmd_setnx,    3, 1);
    cmd_register("SETEX",   cmd_setex,    4, 1);
    cmd_register("MSET",    cmd_mset,    -3, 1);
    cmd_register("MGET",    cmd_mget,    -2, 0);
    cmd_register("INCR",    cmd_incr,     2, 1);
    cmd_register("INCRBY",  cmd_incrby,   3, 1);
    cmd_register("DECR",    cmd_decr,     2, 1);
    cmd_register("DECRBY",  cmd_decrby,   3, 1);
    cmd_register("APPEND",  cmd_append,   3, 1);
    cmd_register("STRLEN",  cmd_strlen,   2, 0);
    /* List */
    cmd_register("LPUSH",   cmd_lpush,   -3, 1);
    cmd_register("RPUSH",   cmd_rpush,   -3, 1);
    cmd_register("LPOP",    cmd_lpop,     2, 1);
    cmd_register("RPOP",    cmd_rpop,     2, 1);
    cmd_register("LRANGE",  cmd_lrange,   4, 0);
    cmd_register("LLEN",    cmd_llen,     2, 0);
    cmd_register("LINDEX",  cmd_lindex,   3, 0);
    cmd_register("LSET",    cmd_lset,     4, 1);
    cmd_register("LREM",    cmd_lrem,     4, 1);
    cmd_register("LTRIM",   cmd_ltrim,    4, 1);
    /* Hash */
    cmd_register("HSET",    cmd_hset,    -4, 1);
    cmd_register("HGET",    cmd_hget,     3, 0);
    cmd_register("HMGET",   cmd_hmget,   -3, 0);
    cmd_register("HGETALL", cmd_hgetall,  2, 0);
    cmd_register("HDEL",    cmd_hdel,    -3, 1);
    cmd_register("HEXISTS", cmd_hexists,  3, 0);
    cmd_register("HLEN",    cmd_hlen,     2, 0);
    cmd_register("HKEYS",   cmd_hkeys,    2, 0);
    cmd_register("HVALS",   cmd_hvals,    2, 0);
    cmd_register("HINCRBY", cmd_hincrby,  4, 1);
    /* Set */
    cmd_register("SADD",      cmd_sadd,      -3, 1);
    cmd_register("SREM",      cmd_srem,      -3, 1);
    cmd_register("SMEMBERS",  cmd_smembers,   2, 0);
    cmd_register("SISMEMBER", cmd_sismember,  3, 0);
    cmd_register("SCARD",     cmd_scard,      2, 0);
    cmd_register("SUNION",    cmd_sunion,    -2, 0);
    cmd_register("SINTER",    cmd_sinter,    -2, 0);
    cmd_register("SDIFF",     cmd_sdiff,     -2, 0);
}
