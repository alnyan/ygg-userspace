#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (unlink(argv[1]) != 0) {
        perror(argv[1]);
    }
    return 0;
}
