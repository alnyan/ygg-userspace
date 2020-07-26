#include <sys/ioctl.h>
#include <termios.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>

static void statusline(size_t height, size_t scroll, size_t lines, int is_eof) {
    printf("\033[%lu;1f\033[7m", height + 1);
    if (lines >= height) {
        printf("lines %lu-%lu", lines - height - scroll, lines - scroll);
    } else {
        printf("lines %lu-%lu", 0L, lines);
    }

    if (is_eof) {
        printf("/%lu ", lines);
        if (scroll) {
            size_t perc = (100 * (lines - scroll)) / lines;
            printf("(%lu%%)", perc);
        } else {
            fputs("(END)", stdout);
        }
    }

    fputs("\033[0m\r", stdout);
    fflush(stdout);
}

static void print_line(const char *line, size_t width, int newline) {
    size_t printed = 0;
    int escape = 0;
    for (size_t j = 0; line[j] && line[j] != '\n'; ++j) {
        if (printed == width - 2) {
            // >> arrow
            fputc(175, stdout);
            break;
        }
        fputc(line[j], stdout);
        if (line[j] == '\033') {
            escape = 1;
        }
        if (escape) {
            if (isalpha(line[j])) {
                escape = 0;
            }
        } else if (isprint(line[j])) {
            ++printed;
        }
    }
    if (newline) {
        fputc('\n', stdout);
    }
}

static void reprint(const char *buf, size_t start, size_t count, size_t limit, size_t width) {
    // Clear screen
    fputs("\033[2J\033[1;1f", stdout);

    // Assuming cursor position is at start
    for (size_t i = start; i < start + count && i < limit; ++i) {
        print_line(&buf[i * 256], width, 1);
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
    int eof_reached;
    FILE *input;
    int tty_fd;

    input = stdin;

    if (argc == 2 && strcmp(argv[1], "-")) {
        input = fopen(argv[1], "r");
        if (!input) {
            perror(argv[1]);
            return -1;
        }
    } else if (argc > 2) {
        fprintf(stderr, "usage: %s [FILENAME]\n", argv[0]);
        return -1;
    }

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
    // TODO: fix this: ICANON is lflag, not iflag!
    termios.c_iflag &= ~ICANON;
    termios.c_lflag &= ~(ECHO | ECHONL | ECHOE | ECHOK | ICANON);

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
        if (fgets(&buf[lines * 256], 256, input) == NULL) {
            break;
        }
        ++lines;
    }

    scroll = 0;
    eof_reached = 0;

    reprint(buf, 0, lines, lines, winsz.ws_col);
    statusline(height, scroll, lines, eof_reached);

    while ((len = read(tty_fd, key, sizeof(key))) > 0) {
        if (key[0] == 'q') {
            break;
        }

        if (key[0] == 'j') {
            if (!scroll) {
                if (eof_reached) {
                    continue;
                }
                // Just read the line and print it at the end
                if (lines >= cap - 1) {
                    // Grow a single page to the buffer
                    cap += height;
                    buf = realloc(buf, cap * 256);
                    if (!buf) {
                        return -1;
                    }
                }

                // End of file reached?
                if (fgets(&buf[lines * 256], 256, input) == NULL) {
                    eof_reached = 1;
                    statusline(height, scroll, lines, eof_reached);
                    continue;
                }

                fputs("\033[2K", stdout);
                print_line(&buf[lines * 256], winsz.ws_col, 1);
                ++lines;
                statusline(height, scroll, lines, eof_reached);
            } else {
                --scroll;
                reprint(buf, lines - height - scroll, height, lines, winsz.ws_col);
                statusline(height, scroll, lines, eof_reached);
            }

            continue;
        } else if (key[0] == 'k') {
            if (lines >= height) {
                size_t max_scroll = lines - height;
                if (scroll < max_scroll) {
                    ++scroll;

                    reprint(buf, lines - height - scroll, height, lines, winsz.ws_col);
                    statusline(height, scroll, lines, eof_reached);
                }
            }
        } else if (key[0] == 'g') {
            if (lines >= height) {
                scroll = lines - height;
                reprint(buf, 0, height, lines, winsz.ws_col);
                statusline(height, scroll, lines, eof_reached);
            }
        } else if (key[0] == 'G') {
            // Read the file until end
            while (!eof_reached) {
                if (lines >= cap - 1) {
                    // Grow a single page to the buffer
                    cap += height;
                    buf = realloc(buf, cap * 256);
                    if (!buf) {
                        return -1;
                    }
                }

                // End of file reached?
                if (fgets(&buf[lines * 256], 256, input) == NULL) {
                    eof_reached = 1;
                    break;
                }
                ++lines;
            }

            scroll = 0;
            if (lines >= height) {
                reprint(buf, lines - height, height, lines, winsz.ws_col);
            } else {
                reprint(buf, 0, height, lines, winsz.ws_col);
            }
            statusline(height, scroll, lines, eof_reached);
        }
    }

    tcsetattr(tty_fd, TCSANOW, &old_termios);
    close(tty_fd);
    fputs("\033[2J\033[1;1f", stdout);

    return 0;
}
