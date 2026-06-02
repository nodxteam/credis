#include "core/server.h"
#include "core/config.h"

int main(int argc, char *argv[]) {
    Config cfg;
    config_defaults(&cfg);

    if (argc > 1 && !config_load(&cfg, argv[1]))
        fprintf(stderr, "Warning: could not load '%s', using defaults\n", argv[1]);

    printf("=== credis ===\n");
    config_print(&cfg);
    putchar('\n');

    server_start(&cfg);
    return 0;
}
