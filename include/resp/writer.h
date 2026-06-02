#ifndef RESP_WRITER_H
#define RESP_WRITER_H

#include "net/io.h"

/* RESP response builders */
void resp_ok(IoCtx *io);
void resp_status(IoCtx *io, const char *s);
void resp_err(IoCtx *io, const char *msg);
void resp_errf(IoCtx *io, const char *fmt, ...);
void resp_int(IoCtx *io, long long n);
void resp_bulk(IoCtx *io, const char *s, int len);  /* $N\r\n...\r\n */
void resp_null_bulk(IoCtx *io);                     /* $-1\r\n */
void resp_array_hdr(IoCtx *io, int count);          /* *N\r\n */
void resp_null_array(IoCtx *io);                    /* *-1\r\n */

#endif
