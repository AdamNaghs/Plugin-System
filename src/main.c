#include "../include/core.h"
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <plugin_folder>\n", argv[0]);
        return 1;
    }

    Core c;
    core_init(&c, 0, argv[1]);
    core_run(&c);
    core_shutdown(&c);
    return 0;
}
