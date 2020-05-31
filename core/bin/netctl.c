#include <ygg/netctl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static int nc_addr_show(int argc, char **argv) {
    return -1;
}

static int nc_addr(int argc, char **argv) {
    if (argc == 0) {
        return nc_addr_show(argc, argv);
    }

    fprintf(stderr, "addr: unknown command: %s\n", argv[0]);
    return -1;
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        fprintf(stderr, "Usage: %s COMMAND [ ARGS ... ]\n", argv[0]);
        fprintf(stderr, "Possible commands:\n");
        fprintf(stderr, "  if   Interface control\n");
        fprintf(stderr, "  addr Address control\n");
        return -1;
    }

    if (!strcmp(argv[1], "addr")) {
        return nc_addr(argc - 2, &argv[2]);
    }

    return 0;
}
