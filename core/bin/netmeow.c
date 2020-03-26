#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

int main(int argc, char **argv) {
    char buf[4096];
    struct sockaddr_in sa;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    size_t salen;

    if (fd < 0) {
        perror("socket()");
        return -1;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(60);
    sa.sin_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) != 0) {
        perror("bind()");
        return -1;
    }

    while (1) {
        ssize_t d = recvfrom(fd, buf, sizeof(buf), (struct sockaddr *) &sa, &salen);
        if (d < 0) {
            perror("recvfrom()");
            break;
        }
        write(STDOUT_FILENO, buf, d);
    }

    return 0;
}
