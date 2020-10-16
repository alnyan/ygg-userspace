#include <sys/stat.h>
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("usage: mkdir <path> ...\n");
        return -1;
    }

    int res = 0;
    int ores;

    for (int i = 1; i < argc; ++i) {
        ores = mkdir(argv[i], 0777);
        if (ores != 0) {
            warn("mkdir(%s)", argv[i]);
        }
        res += ores;
    }

    return res;
}
