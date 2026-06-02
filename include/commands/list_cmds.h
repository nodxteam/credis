#ifndef COMMANDS_LIST_H
#define COMMANDS_LIST_H

#include "commands/dispatcher.h"

void cmd_lpush(Client *c);
void cmd_rpush(Client *c);
void cmd_lpop(Client *c);
void cmd_rpop(Client *c);
void cmd_lrange(Client *c);
void cmd_llen(Client *c);
void cmd_lindex(Client *c);
void cmd_lset(Client *c);
void cmd_lrem(Client *c);
void cmd_ltrim(Client *c);

#endif
