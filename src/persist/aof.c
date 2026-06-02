#include "persist/aof.h"
#include "store/db.h"
#include "commands/dispatcher.h"
#include "resp/parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

AofLog *aof_open(const char *path) {
    AofLog *aof = (AofLog *)calloc(1, sizeof(AofLog));
    aof->fp = fopen(path, "a+b");
    if (!aof->fp) { free(aof); return NULL; }
    mutex_init(&aof->mutex);
    return aof;
}

void aof_append(AofLog *aof, char **argv, int argc) {
    if (!aof || !aof->fp) return;
    mutex_lock(&aof->mutex);
    fprintf(aof->fp, "*%d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        int len = (int)strlen(argv[i]);
        fprintf(aof->fp, "$%d\r\n%s\r\n", len, argv[i]);
    }
    fflush(aof->fp);
    mutex_unlock(&aof->mutex);
}

int aof_replay(AofLog *aof, DB *db) {
    if (!aof || !aof->fp) return 0;

    rewind(aof->fp);
    aof->replaying = 1;
    int replayed = 0;

    /* Create a fake client backed by a pipe so the parser can read from file */
    /* Simpler approach: read lines directly from the AOF file */
    char line[1024];
    char *argv[MAX_CMD_ARGS];
    int argc = 0, expected_argc = 0, bulk_len = 0;
    int state = 0; /* 0=expect *N, 1=expect $N, 2=expect bulk data */
    int arg_idx = 0;

    while (fgets(line, sizeof(line), aof->fp)) {
        /* Strip \r\n */
        line[strcspn(line, "\r\n")] = '\0';
        if (!line[0]) continue;

        if (state == 0) {
            if (line[0] != '*') continue;
            expected_argc = atoi(line + 1);
            if (expected_argc <= 0 || expected_argc > MAX_CMD_ARGS) continue;
            argc = 0; arg_idx = 0;
            state = 1;
        } else if (state == 1) {
            if (line[0] != '$') { state = 0; continue; }
            bulk_len = atoi(line + 1);
            state = 2;
        } else if (state == 2) {
            argv[arg_idx] = strndup(line, (size_t)bulk_len);
            arg_idx++;
            argc++;
            if (argc == expected_argc) {
                /* Execute command */
                Client fake;
                memset(&fake, 0, sizeof(fake));
                fake.db  = db;
                fake.aof = aof;
                /* Use a pipe/dummy IO — redirect output to /dev/null */
#ifdef _WIN32
                fake.io.fd = INVALID_SOCKET;
#else
                fake.io.fd = open("/dev/null", 1); /* write-only */
#endif
                fake.argc = argc;
                for (int i = 0; i < argc; i++) fake.argv[i] = argv[i];
                cmd_dispatch(&fake);
                for (int i = 0; i < argc; i++) { free(argv[i]); fake.argv[i] = NULL; }
                fake.argc = 0;
#ifndef _WIN32
                if (fake.io.fd >= 0) close(fake.io.fd);
#endif
                replayed++;
                state = 0;
            } else {
                state = 1;
            }
        }
    }

    aof->replaying = 0;
    return replayed;
}

void aof_close(AofLog *aof) {
    if (!aof) return;
    if (aof->fp) fclose(aof->fp);
    mutex_destroy(&aof->mutex);
    free(aof);
}
