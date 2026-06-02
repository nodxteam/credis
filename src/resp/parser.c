#include "resp/parser.h"
#include "resp/writer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Buffered reader helpers ─────────────────────────────────────────────── */

static int buf_fill(Client *c) {
    int n = io_read(&c->io, c->rbuf, RBUF_SIZE);
    if (n <= 0) return -1;
    c->rbuf_pos = 0;
    c->rbuf_len = (size_t)n;
    return 0;
}

static int buf_getc(Client *c, char *out) {
    if (c->rbuf_pos >= c->rbuf_len)
        if (buf_fill(c) < 0) return -1;
    *out = c->rbuf[c->rbuf_pos++];
    return 0;
}

/* Read until \n, strip trailing \r\n. */
static int buf_readline(Client *c, char *out, size_t sz) {
    size_t i = 0;
    char ch;
    while (i < sz - 1) {
        if (buf_getc(c, &ch) < 0) return -1;
        if (ch == '\n') {
            if (i > 0 && out[i-1] == '\r') i--;
            out[i] = '\0';
            return 0;
        }
        out[i++] = ch;
    }
    return -1; /* line too long */
}

/* Read exactly n bytes. */
static int buf_readn(Client *c, char *out, size_t n) {
    size_t got = 0;
    while (got < n) {
        if (c->rbuf_pos >= c->rbuf_len)
            if (buf_fill(c) < 0) return -1;
        size_t avail = c->rbuf_len - c->rbuf_pos;
        size_t take = avail < (n - got) ? avail : (n - got);
        memcpy(out + got, c->rbuf + c->rbuf_pos, take);
        c->rbuf_pos += take;
        got += take;
    }
    return 0;
}

/* ── Parser ──────────────────────────────────────────────────────────────── */

void client_free_args(Client *c) {
    for (int i = 0; i < c->argc; i++) {
        free(c->argv[i]);
        c->argv[i] = NULL;
    }
    c->argc = 0;
}

static int parse_inline(Client *c, const char *line) {
    /* Split by whitespace */
    c->argc = 0;
    const char *p = line;
    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;
        const char *start = p;
        while (*p && *p != ' ') p++;
        size_t len = (size_t)(p - start);
        c->argv[c->argc] = (char *)malloc(len + 1);
        memcpy(c->argv[c->argc], start, len);
        c->argv[c->argc][len] = '\0';
        c->arglens[c->argc] = (int)len;
        c->argc++;
        if (c->argc >= MAX_CMD_ARGS) break;
    }
    return c->argc > 0 ? 0 : -1;
}

int resp_read_cmd(Client *c) {
    client_free_args(c);

    char line[1024];
    if (buf_readline(c, line, sizeof(line)) < 0) return -1;
    if (!line[0]) return -1;

    if (line[0] != '*') {
        /* Inline command */
        return parse_inline(c, line);
    }

    /* Multi-bulk: *<argc>\r\n */
    int argc = atoi(line + 1);
    if (argc <= 0 || argc > MAX_CMD_ARGS) return -1;

    for (int i = 0; i < argc; i++) {
        if (buf_readline(c, line, sizeof(line)) < 0) return -1;
        if (line[0] != '$') return -1;
        int blen = atoi(line + 1);
        if (blen < 0) { c->argv[i] = strdup(""); c->arglens[i] = 0; continue; }
        if (blen > 512 * 1024 * 1024) return -1; /* 512MB hard limit */

        c->argv[i] = (char *)malloc((size_t)blen + 1);
        if (buf_readn(c, c->argv[i], (size_t)blen) < 0) { free(c->argv[i]); return -1; }
        c->argv[i][blen] = '\0';
        c->arglens[i] = blen;

        /* Consume trailing \r\n */
        char crlf[2];
        if (buf_readn(c, crlf, 2) < 0) return -1;
    }

    c->argc = argc;
    return 0;
}
