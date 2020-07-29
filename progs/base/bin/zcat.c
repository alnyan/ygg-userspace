#include <stdio.h>
#include <zlib.h>

int main(int argc, char **argv) {
    gzFile file;
    char buf[1024];
    ssize_t bread;

    if (argc != 2) {
        fprintf(stderr, "usage: %s FILENAME\n", argv[0]);
        return 1;
    }

    file = gzopen(argv[1], "r");
    if (!file) {
        perror(argv[1]);
        return 1;
    }

    while ((bread = gzread(file, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, bread, stdout);
    }

    gzclose(file);

    return 0;
}
