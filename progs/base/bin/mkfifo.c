#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s FILENAME\n", argv[0]);
        return -1;
    }

    if (mknod(argv[1], 0644 | S_IFIFO, 0) != 0) {
        perror(argv[1]);
        return -1;
    }

    return 0;
}
