#include <sys/ioctl.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

static void reprint(const char *buf, size_t start, size_t count, size_t limit) {
    // Clear screen
    fputs("\033[2J\033[1;1f", stdout);
    fflush(stdout);

    // Assuming cursor position is at start
    for (size_t i = start; i < start + count && i < limit; ++i) {
        const char *line = &buf[i * 256];
        printf("%s", line);
    }
    fflush(stdout);
}

int main(int argc, char **argv) {
    // Alloc initial buffer
    struct termios termios, old_termios;
    struct winsize winsz;
    size_t height;
    ssize_t len;
    char *buf;
    char key[256];
    size_t lines, cap, scroll;
    int tty_fd;

    if (!isatty(STDOUT_FILENO)) {
        fprintf(stderr, "stdout is not a tty\n");
        return -1;
    }

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz) != 0) {
        perror("window size");
        return -1;
    }
    height = winsz.ws_row - 1;

    // For keyboard input
    tty_fd = open("/dev/tty", O_RDONLY, 0);
    if (tty_fd < 0) {
        perror("/dev/tty");
        return -1;
    }

    // Set non-canonical term mode
    if (tcgetattr(tty_fd, &termios) != 0) {
        perror("tcgetattr()");
        return -1;
    }

    memcpy(&old_termios, &termios, sizeof(struct termios));
    termios.c_iflag &= ~ICANON;
    termios.c_lflag &= ~(ECHO | ECHONL | ECHOE | ECHOK);

    if (tcsetattr(tty_fd, TCSANOW, &termios) != 0) {
        perror("tcsetattr()");
        return -1;
    }

    // Read lines until screen full
    lines = 0;
    cap = height;
    // Max line length: 256
    buf = malloc(256 * cap);

    while (lines < height) {
        if (fgets(&buf[lines * 256], 256, stdin) == NULL) {
            break;
        }
        ++lines;
    }

    scroll = 0;

    reprint(buf, 0, lines, lines);

    while ((len = read(tty_fd, key, sizeof(key))) > 0) {
        if (key[0] == 'q') {
            break;
        }

        if (key[0] == 'j') {
            if (!scroll) {
                // Just read the line and print it at the end
                if (lines == cap) {
                    // Grow a single page to the buffer
                    cap += 256 * height;
                    buf = realloc(buf, cap);
                    if (!buf) {
                        return -1;
                    }
                }

                // End of file reached?
                if (fgets(&buf[lines * 256], 256, stdin) == NULL) {
                    continue;
                }

                printf("%s", &buf[lines * 256]);
                fflush(stdout);

                ++lines;
            } else {
                --scroll;
                reprint(buf, lines - height - scroll, height, lines);
            }

            continue;
        } else if (key[0] == 'k') {
            if (lines >= height) {
                size_t max_scroll = lines - height;
                if (scroll < max_scroll) {
                    ++scroll;

                    reprint(buf, lines - height - scroll, height, lines);
                }
            }
        }
    }

    tcsetattr(tty_fd, TCSANOW, &old_termios);
    close(tty_fd);

    return 0;
}
