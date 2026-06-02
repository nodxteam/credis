#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include "commands/dispatcher.h"

/*
 * Read one complete RESP command from the client's buffered stream.
 * Supports multi-bulk (*N\r\n$N\r\n...) and inline (PING\r\n).
 * Fills c->argc and c->argv[].
 * Returns  0 on success.
 * Returns -1 on disconnect or read error.
 */
int resp_read_cmd(Client *c);

#endif
