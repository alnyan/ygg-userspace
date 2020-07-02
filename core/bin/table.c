#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

int main(int argc, char **argv) {
    // 25 x 3
    size_t width;
    size_t cols = 4;
    struct winsize winsz;
    int index = 0;
    char linebuf[1024];

    if (!isatty(STDOUT_FILENO)) {
        fprintf(stderr, "stdout is not a TTY\n");
        return -1;
    }

    // Get terminal width
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winsz) != 0) {
        perror("stdout");
        return -1;
    }

    width = (winsz.ws_col - 1) / cols;

    while (fgets(linebuf, sizeof(linebuf), stdin)) {
        // Remove trailing \n
        char *p = strchr(linebuf, '\n');
        if (p && *linebuf) {
            *p = 0;
        }

        // Skip leading whitespace
        p = linebuf;
        while (isspace(*p)) {
            ++p;
        }

        size_t len = strlen(p);
        if (len > width) {
            len = width;
        }

        fwrite(p, 1, len, stdout);
        for (size_t i = len; i < width; ++i) {
            fputc(' ', stdout);
        }
        if ((++index) == cols) {
            fputc('\n', stdout);
            index = 0;
        }
    }
    if (index != 0) {
        fputc('\n', stdout);
    }
    fflush(stdout);
    return 0;
}
