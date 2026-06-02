#include "core/server.h"
#include "commands/dispatcher.h"
#include "resp/parser.h"
#include "resp/writer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Background expiry thread ────────────────────────────────────────────── */

#ifdef _WIN32
static DWORD WINAPI expiry_thread(LPVOID arg)
#else
static void *expiry_thread(void *arg)
#endif
{
    Server *srv = (Server *)arg;
    unsigned int interval_ms = (srv->config->hz > 0)
        ? (unsigned int)(1000 / srv->config->hz) : 100;
    for (;;) {
        db_purge_expired(srv->db);
#ifdef _WIN32
        Sleep(interval_ms);
#else
        usleep((useconds_t)interval_ms * 1000);
#endif
    }
#ifndef _WIN32
    return NULL;
#else
    return 0;
#endif
}

/* ── Per-connection worker ───────────────────────────────────────────────── */

static void handle_client(void *arg) {
    Client *c = (Client *)arg;

    for (;;) {
        if (resp_read_cmd(c) < 0) break;
        cmd_dispatch(c);
        client_free_args(c);
    }

    io_close(&c->io);
    free(c);
}

/* ── Server entry point ──────────────────────────────────────────────────── */

void server_start(Config *config) {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    /* AOF replay */
    DB     *db  = db_create();
    AofLog *aof = NULL;

    if (config->aof_enabled) {
        aof = aof_open(config->aof_file);
        if (aof) {
            int replayed = aof_replay(aof, db);
            if (replayed > 0)
                printf("AOF: replayed %d commands\n", replayed);
        }
    }

    cmd_register_all();

    ThreadPool *pool = pool_create(config->max_clients);

    Server srv = { config, pool, db, aof, INVALID_SOCK };

    /* Start background expiry thread */
#ifdef _WIN32
    CreateThread(NULL, 0, expiry_thread, &srv, 0, NULL);
#else
    {
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&tid, &attr, expiry_thread, &srv);
        pthread_attr_destroy(&attr);
    }
#endif

    sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == INVALID_SOCK) { fprintf(stderr, "socket() failed\n"); goto cleanup; }
    srv.fd = fd;

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr; memset(&addr,0,sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((unsigned short)config->port);

    if (bind(fd,(struct sockaddr *)&addr,sizeof(addr))<0 || listen(fd,512)<0) {
        fprintf(stderr, "bind/listen failed on port %d\n", config->port);
        sock_close(fd); goto cleanup;
    }

    printf("credis listening on %s:%d\n", config->bind_addr, config->port);
    printf("Connect with: redis-cli -p %d\n\n", config->port);

    for (;;) {
        struct sockaddr_in caddr;
#ifdef _WIN32
        int alen = sizeof(caddr);
#else
        socklen_t alen = sizeof(caddr);
#endif
        sock_t client = accept(fd,(struct sockaddr *)&caddr,&alen);
        if (client == INVALID_SOCK) continue;

        Client *c = (Client *)calloc(1, sizeof(Client));
        io_init(&c->io, client);
        c->db  = db;
        c->aof = aof;
        inet_ntop(AF_INET, &caddr.sin_addr, c->client_ip, sizeof(c->client_ip));

        pool_submit(pool, handle_client, c);
    }

    sock_close(fd);

cleanup:
    pool_destroy(pool);
    db_destroy(db);
    if (aof) aof_close(aof);
#ifdef _WIN32
    WSACleanup();
#endif
}
