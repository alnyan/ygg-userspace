#include <sys/debug.h>

#include "object.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        return -1;
    }
    const char *path = argv[1];
    struct object *program = object_open(path);
    if (!program) {
        return -1;
    }

    object_load(program);

    object_entry_get(program)(0);

    while (1);

    return 0;
}
