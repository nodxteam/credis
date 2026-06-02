#include "net/io.h"
void io_init(IoCtx *io, sock_t fd) { io->fd = fd; }
int  io_read(IoCtx *io, char *buf, int len) { return recv(io->fd, buf, len, 0); }
int  io_write(IoCtx *io, const char *buf, int len) { return send(io->fd, buf, len, 0); }
void io_write_all(IoCtx *io, const char *buf, size_t len) {
    size_t s=0; while(s<len){int n=io_write(io,buf+s,(int)(len-s));if(n<=0)break;s+=(size_t)n;}
}
void io_close(IoCtx *io) { sock_close(io->fd); }
