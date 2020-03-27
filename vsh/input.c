#include <sys/select.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "video.h"
#include "input.h"

static fd_set input_fds;
static struct termios tc_old, tc;
static struct timeval input_tv;
static int keyboard_fd = STDIN_FILENO;
static int mouse_fd;
int cursor_x = 0, cursor_y = 0;

void cursor_move(int16_t dx, int16_t dy) {
    cursor_x += dx;
    cursor_y -= dy;

    if (cursor_x < 0) {
        cursor_x = 0;
    } else if (cursor_x >= fb_mode.width) {
        cursor_x = fb_mode.width - 1;
    }
    if (cursor_y < 0) {
        cursor_y = 0;
    } else if (cursor_y >= fb_mode.height) {
        cursor_y = fb_mode.height - 1;
    }
}

int input_wait_event(struct input_event *ev) {
    int res;
    ssize_t len;
    char buf[128];

    FD_ZERO(&input_fds);
    FD_SET(keyboard_fd, &input_fds);
    FD_SET(mouse_fd, &input_fds);
    input_tv.tv_sec = 0;
    input_tv.tv_usec = 10000;

    res = select(mouse_fd + 1, &input_fds, NULL, NULL, &input_tv);

    if (res < 0) {
        return -1;
    }

    if (res > 0) {
        if (FD_ISSET(keyboard_fd, &input_fds)) {
            // Read a single key event
            len = read(keyboard_fd, buf, 1);

            if (len < 0) {
                return -1;
            }

            ev->type = IN_KEY_DOWN;
            ev->key = buf[0];

            return 1;
        }
        if (FD_ISSET(mouse_fd, &input_fds)) {
            len = read(mouse_fd, buf, 5);

            if (len != 5) {
                return 0;
            }

            char type = buf[0];

            switch (type) {
            case 'd':
                ev->type = IN_MOUSE_MOVE;
                ev->mouse_move.dx = *(int16_t *) &buf[1];
                ev->mouse_move.dy = *(int16_t *) &buf[3];
                cursor_move(ev->mouse_move.dx, ev->mouse_move.dy);
                return 1;
            case 'b':
                ev->type = buf[3] ? IN_BUTTON_DOWN : IN_BUTTON_UP;
                ev->button = buf[1];
                return 1;
            default:
                // Skip unknown events
                return 0;
            }
        }
    }

    return 0;
}

int input_init(void) {
    if (tcgetattr(keyboard_fd, &tc_old)) {
        perror("tcgetattr()");
        return -1;
    }
    memcpy(&tc, &tc_old, sizeof(struct termios));
    // No canonical mode
    tc.c_iflag &= ~ICANON;
    // No echo
    tc.c_lflag = 0;

    if (tcsetattr(keyboard_fd, TCSANOW, &tc) != 0) {
        perror("tcsetattr()");
        return -1;
    }

    mouse_fd = open("/dev/ps2aux", O_RDONLY, 0);

    if (mouse_fd < 0) {
        perror("open(/dev/ps2aux)");
        // Restore old terminal behavior
        tcsetattr(keyboard_fd, TCSANOW, &tc_old);
        return -1;
    }

    return 0;
}

void input_close(void) {
    tcsetattr(keyboard_fd, TCSANOW, &tc_old);
    close(mouse_fd);
}
