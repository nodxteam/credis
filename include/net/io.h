#ifndef NET_IO_H
#define NET_IO_H

#include "platform.h"

typedef struct { sock_t fd; } IoCtx;

void io_init(IoCtx *io, sock_t fd);
int  io_read(IoCtx *io, char *buf, int len);
int  io_write(IoCtx *io, const char *buf, int len);
void io_write_all(IoCtx *io, const char *buf, size_t len);
void io_close(IoCtx *io);

#endif
