#include <sys/socket.h>
#include <string.h>
#include <ygg/un.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
    struct sockaddr_un un;
    ssize_t rd;
    int fd;
    char buf[1024];

    if (argc != 2) {
        fprintf(stderr, "usage: %s SOCKET\n", argv[0]);
        return -1;
    }

    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, argv[1]);

    if ((fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }

    if (connect(fd, (struct sockaddr *) &un, sizeof(un)) != 0) {
        perror("connect()");
        return -1;
    }

    while ((rd = recv(fd, buf, sizeof(buf), 0)) > 0) {
        write(STDOUT_FILENO, buf, rd);
    }

    close(fd);

    return 0;
}
