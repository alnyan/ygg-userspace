#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>

int main(int argc, char **argv) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);

    if (sock < 0) {
        perror("socket()");
        return -1;
    }

    close(sock);

    return 0;
}
