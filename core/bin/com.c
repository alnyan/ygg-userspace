#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>

// TODO: baud rate support in kernel
static int ready = 0, there_fd;
static struct termios here_tc, here_tc_old;
static struct termios there_tc, there_tc_old;

static void sig_handler(int signum) {
    // Restore old termios
    if (ready) {
        printf("Interrupted\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &here_tc_old);
        tcsetattr(there_fd, TCSANOW, &there_tc_old);
    }
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: com /dev/ttySn\n");
        return -1;
    }

    signal(SIGINT, sig_handler);

    char buf[512];
    fd_set fds;
    ssize_t len;
    int res;

    if ((there_fd = open(argv[1], O_RDWR, 0)) < 0) {
        perror(argv[1]);
        return -1;
    }

    if (tcgetattr(STDIN_FILENO, &here_tc_old)) {
        perror("stdin");
        close(there_fd);
        return -1;
    }

    if (tcgetattr(there_fd, &there_tc_old) != 0) {
        perror(argv[1]);
        close(there_fd);
        return -1;
    }

    ready = 1;

    // Make them both raw
    memcpy(&here_tc, &here_tc_old, sizeof(struct termios));
    memcpy(&there_tc, &there_tc_old, sizeof(struct termios));

    here_tc.c_iflag &= ~ICANON;
    here_tc.c_lflag &= ~ECHO;
    there_tc.c_iflag &= ~ICANON;
    there_tc.c_lflag &= ~ECHO;

    res = -1;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &here_tc) != 0) {
        perror("stdin");
        goto cleanup;
    }

    if (tcsetattr(there_fd, TCSANOW, &there_tc) != 0) {
        perror(argv[1]);
        goto cleanup;
    }

    while (1) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        FD_SET(there_fd, &fds);

        if ((res = select(there_fd + 1, &fds, NULL, NULL, NULL)) < 0) {
            goto cleanup;
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            if ((len = read(STDIN_FILENO, buf, sizeof(buf))) <= 0) {
                if (len != 0) {
                    perror("read()");
                    res = -1;
                }
                goto cleanup;
            }

            if (write(there_fd, buf, len) != len) {
                perror("write()");
                res = -1;
                goto cleanup;
            }
        }
        if (FD_ISSET(there_fd, &fds)) {
            if ((len = read(there_fd, buf, sizeof(buf))) <= 0) {
                if (len == 0) {
                    printf("Remote side hang up\n");
                } else {
                    perror("read()");
                    res = -1;
                }
                goto cleanup;
            }

            if (write(STDOUT_FILENO, buf, len) != len) {
                perror("write()");
                res = -1;
                goto cleanup;
            }
        }
    }

    res = 0;
cleanup:
    tcsetattr(there_fd, TCSANOW, &there_tc_old);
    tcsetattr(STDIN_FILENO, TCSANOW, &here_tc_old);
    close(there_fd);
    ready = 0;

    return 0;
}
