#include <stdio.h>

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("usage: mkdir <path> ...\n");
        return -1;
    }

    int res = 0;

    for (int i = 1; i < argc; ++i) {
        res += mkdir(argv[i], 0755);
    }

    return res;
}
