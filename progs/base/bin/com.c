#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define OPT_HEX     (1 << 0)

static void usage(char **argv) {
    fprintf(stderr, "usage: %s [-x] DEVICE\n", argv[0]);
}

int main(int argc, char **argv) {
    struct termios tc0r, tc1r, tc0l, tc1l;
    int flags = 0;
    int fd, res;
    ssize_t bread;
    struct timeval tv;
    char buf[1024];
    fd_set fds;
    static const char *opts = "x";
    char c;

    while ((c = getopt(argc, argv, opts)) != -1) {
        switch (c) {
        case 'x':
            flags |= OPT_HEX;
            break;
        case '?':
            usage(argv);
            return -1;
        }
    }

    if (optind != argc - 1) {
        usage(argv);
        return -1;
    }

    // Setup local terminal
    if (tcgetattr(STDIN_FILENO, &tc0l) != 0) {
        perror("get local termios");
        return -1;
    }
    tc1l = tc0l;
    tc1l.c_lflag &= ~ICANON;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tc1l) != 0) {
        perror("set local termios");
        return -1;
    }

    if ((fd = open(argv[optind], O_RDWR, 0)) < 0) {
        perror(argv[optind]);
        goto cleanup_local;
    }

    // Setup remote terminal
    if (tcgetattr(fd, &tc0r) != 0) {
        perror("get remote termios");
        goto cleanup_local;
    }
    tc1r = tc0r;
    tc1r.c_lflag &= ~ICANON;
    if (tcsetattr(fd, TCSANOW, &tc1r) != 0) {
        perror("set remote termios");
        goto cleanup_local;
    }

    while (1) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(fd, &fds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        res = select(fd + 1, &fds, NULL, NULL, &tv);

        if (res < 0) {
            perror("select()");
            goto cleanup_remote;
        }
        if (res == 0) {
            continue;
        }

        if (FD_ISSET(fd, &fds)) {
            bread = read(fd, buf, sizeof(buf));
            if (bread <= 0) {
                break;
            }
            if (!(flags & OPT_HEX)) {
                write(STDOUT_FILENO, buf, bread);
            } else {
                for (ssize_t i = 0; i < bread; ++i) {
                    printf("%02hhx", buf[i]);
                }
                fflush(stdout);
            }
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            bread = read(STDIN_FILENO, buf, sizeof(buf));
            int brk = 0;
            if (bread <= 0) {
                break;
            }
            size_t count = 0;
            for (ssize_t i = 0; i < bread; ++i) {
                if (buf[i] == 0x04) {
                    // EOF
                    brk = 1;
                    break;
                }
                ++count;
            }
            write(fd, buf, count);
            if (brk) {
                break;
            }
        }
    }

cleanup_remote:
    tcsetattr(STDIN_FILENO, TCSANOW, &tc0r);
cleanup_local:
    tcsetattr(STDIN_FILENO, TCSANOW, &tc0l);

    return 0;
}
