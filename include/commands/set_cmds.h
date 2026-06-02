#ifndef COMMANDS_SET_H
#define COMMANDS_SET_H

#include "commands/dispatcher.h"

void cmd_sadd(Client *c);
void cmd_srem(Client *c);
void cmd_smembers(Client *c);
void cmd_sismember(Client *c);
void cmd_scard(Client *c);
void cmd_sunion(Client *c);
void cmd_sinter(Client *c);
void cmd_sdiff(Client *c);

#endif
