#include <stdio.h>

int main(int argc, char **argv) {
    char buf[1024];
    FILE *input = stdin;
    FILE *files[argc];

    files[0] = stdout;
    for (int i = 1; i < argc; ++i) {
        files[i] = fopen(argv[i], "w");
        if (!files[i]) {
            perror(argv[i]);
            return -1;
        }
    }

    while (fgets(buf, sizeof(buf), input)) {
        for (int i = 0; i < argc; ++i) {
            fputs(buf, files[i]);
        }
    }
}
