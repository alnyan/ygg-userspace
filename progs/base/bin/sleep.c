#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s SEC\n", argv[0]);
        return -1;
    }

    int res;
    struct timespec t;

    t.tv_sec = atoi(argv[1]);
    t.tv_nsec = 0;

    if ((res = nanosleep(&t, NULL)) != 0) {
        perror("sleep");
        return -1;
    }

    return 0;
}
