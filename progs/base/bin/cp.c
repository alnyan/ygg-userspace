#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s SRC DST\n", argv[0]);
        return -1;
    }

    int fd0, fd1;
    fd0 = open(argv[1], O_RDONLY, 0);
    if (fd0 < 0) {
        perror(argv[1]);
        return -1;
    }
    fd1 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0755);
    if (fd1 < 0) {
        perror(argv[2]);
        return -1;
    }

    ssize_t bread;
    char buf[4096];
    while ((bread = read(fd0, buf, sizeof(buf))) > 0) {
        if (write(fd1, buf, bread) < 0) {
            perror(argv[2]);
            break;
        }
    }

    close(fd0);
    close(fd1);
    return 0;
}
