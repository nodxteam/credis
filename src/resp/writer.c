#include "resp/writer.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void resp_ok(IoCtx *io)           { io_write_all(io, "+OK\r\n", 5); }
void resp_status(IoCtx *io, const char *s) {
    char buf[256]; int n=snprintf(buf,sizeof(buf),"+%s\r\n",s);
    io_write_all(io, buf, (size_t)n);
}
void resp_err(IoCtx *io, const char *msg) {
    char buf[512]; int n=snprintf(buf,sizeof(buf),"-ERR %s\r\n",msg);
    io_write_all(io, buf, (size_t)n);
}
void resp_errf(IoCtx *io, const char *fmt, ...) {
    char msg[512]; va_list ap; va_start(ap,fmt); vsnprintf(msg,sizeof(msg),fmt,ap); va_end(ap);
    resp_err(io, msg);
}
void resp_int(IoCtx *io, long long n) {
    char buf[32]; int len=snprintf(buf,sizeof(buf),":%lld\r\n",n);
    io_write_all(io, buf, (size_t)len);
}
void resp_bulk(IoCtx *io, const char *s, int len) {
    char hdr[32]; int hl=snprintf(hdr,sizeof(hdr),"$%d\r\n",len);
    io_write_all(io, hdr, (size_t)hl);
    io_write_all(io, s, (size_t)len);
    io_write_all(io, "\r\n", 2);
}
void resp_null_bulk(IoCtx *io)    { io_write_all(io, "$-1\r\n", 5); }
void resp_array_hdr(IoCtx *io, int count) {
    char buf[32]; int n=snprintf(buf,sizeof(buf),"*%d\r\n",count);
    io_write_all(io, buf, (size_t)n);
}
void resp_null_array(IoCtx *io)   { io_write_all(io, "*-1\r\n", 5); }
