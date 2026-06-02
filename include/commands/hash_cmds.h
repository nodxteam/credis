#ifndef COMMANDS_HASH_H
#define COMMANDS_HASH_H

#include "commands/dispatcher.h"

void cmd_hset(Client *c);
void cmd_hget(Client *c);
void cmd_hmget(Client *c);
void cmd_hgetall(Client *c);
void cmd_hdel(Client *c);
void cmd_hexists(Client *c);
void cmd_hlen(Client *c);
void cmd_hkeys(Client *c);
void cmd_hvals(Client *c);
void cmd_hincrby(Client *c);

#endif
