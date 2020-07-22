#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s SEC\n", argv[0]);
        return -1;
    }

    return sleep(atoi(argv[1]));
}
