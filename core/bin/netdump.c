#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdio.h>

#define LINE_LENGTH         16

static void line_print(size_t off, const char *line, size_t len) {
    printf("%08x: ", off);
    for (size_t i = 0; i < LINE_LENGTH; ++i) {
        // XXX: This is needed because I didn't implement h/hh modifiers in printf
        uint64_t byte = (uint64_t) line[i] & 0xFF;
        if (i < len) {
            printf("%02x", byte);
        } else {
            printf("  ");
        }
        if (i % 2) {
            printf(" ");
        }
    }
    printf("| ");
    for (size_t i = 0; i < len; ++i) {
        // TODO: isprint?
        if (((uint8_t) line[i]) >= ' ') {
            printf("%c", line[i]);
        } else {
            printf(".");
        }
    }
    printf("\n");
}

void packet_dump(const char *buf, size_t len) {
    char line[LINE_LENGTH];
    size_t linel = 0;
    size_t off = 0;

    for (size_t i = 0; i < len; ++i) {
        line[linel++] = buf[i];
        if (linel == LINE_LENGTH) {
            line_print(off, line, linel);

            off += LINE_LENGTH;
            linel = 0;
        }
    }
    if (linel) {
        line_print(off, line, linel);
    }
}

int main(int argc, char **argv) {
    struct sockaddr sa;
    size_t salen;
    char buf[4096];
    int fd = socket(AF_PACKET, SOCK_RAW, 0);
    fd_set fds;
    struct timeval tv;
    int res;

    if (fd < 0) {
        perror("socket()");
        return -1;
    }

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        res = select(fd + 1, &fds, NULL, NULL, &tv);

        if (res < 0) {
            perror("select()");
        }

        if (res > 0) {
            ssize_t len = recvfrom(fd, buf, sizeof(buf), &sa, &salen);

            if (len < 0) {
                perror("recvfrom()");
                break;
            }

            packet_dump(buf, len);
        } else {
            printf("Timed out\n");
        }
    }

    return 0;
}
