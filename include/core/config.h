#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include "platform.h"

typedef struct {
    int  port;
    char bind_addr[64];
    char aof_file[MAX_PATH_LEN];
    int  aof_enabled;
    int  max_clients;
    int  timeout_sec;
    int  hz;              /* active expiry frequency (times/sec) */
    char loglevel[16];    /* "debug" / "verbose" / "notice" / "warning" */
} Config;

void config_defaults(Config *cfg);
int  config_load(Config *cfg, const char *path);
void config_print(const Config *cfg);

#endif
