#include <sys/mount.h>
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv) {
    int res;

    if (argc != 2) {
        fprintf(stderr, "Usage: umount dir\n");
        return -1;
    }

    if ((res = umount(argv[1])) != 0) {
        err("umount(%s)", argv[1]);
    }

    return 0;
}
