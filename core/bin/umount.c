#include <stdio.h>

int main(int argc, char **argv) {
    int res;

    if (argc != 2) {
        printf("Usage: umount dir\n");
        return -1;
    }

    if ((res = umount(argv[1])) != 0) {
        perror(argv[1]);
        return -1;
    }

    return 0;
}
