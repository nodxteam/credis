#ifndef COMMANDS_STRING_H
#define COMMANDS_STRING_H

#include "commands/dispatcher.h"

void cmd_set(Client *c);
void cmd_get(Client *c);
void cmd_getset(Client *c);
void cmd_setnx(Client *c);
void cmd_setex(Client *c);
void cmd_mset(Client *c);
void cmd_mget(Client *c);
void cmd_incr(Client *c);
void cmd_incrby(Client *c);
void cmd_decr(Client *c);
void cmd_decrby(Client *c);
void cmd_append(Client *c);
void cmd_strlen(Client *c);

#endif
