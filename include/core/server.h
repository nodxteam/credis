#ifndef CORE_SERVER_H
#define CORE_SERVER_H

#include "platform.h"
#include "net/io.h"
#include "core/config.h"
#include "core/thread_pool.h"
#include "store/db.h"
#include "persist/aof.h"

typedef struct {
    Config     *config;
    ThreadPool *pool;
    DB         *db;
    AofLog     *aof;
    sock_t      fd;
} Server;

void server_start(Config *config);

#endif
