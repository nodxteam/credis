#ifndef COMMANDS_GENERIC_H
#define COMMANDS_GENERIC_H

#include "commands/dispatcher.h"

void cmd_ping(Client *c);
void cmd_quit(Client *c);
void cmd_del(Client *c);
void cmd_exists(Client *c);
void cmd_type(Client *c);
void cmd_expire(Client *c);
void cmd_expireat(Client *c);
void cmd_ttl(Client *c);
void cmd_persist(Client *c);
void cmd_keys(Client *c);
void cmd_dbsize(Client *c);
void cmd_flushdb(Client *c);

#endif
