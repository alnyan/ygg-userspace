#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

static int running = 1;

//static int mx = 40;
//static int my = 12;
//static double mx_d = 40, my_d = 12;

static int con_width, con_height;

static void signal_handle(int signum) {
    running = 0;
}

//static void kb_handle(char *buf, size_t len) {
//    if (buf[0] == 'q') {
//        running = 0;
//    }
//}
//
//static void ms_handle(char *buf, size_t len) {
//    for (size_t i = 0; i < len / 5; ++i) {
//        char type = buf[i * 5 + 0];
//
//        switch (type) {
//        case 'd': {
//                int16_t dx = *(int16_t *) &buf[i * 5 + 1];
//                int16_t dy = *(int16_t *) &buf[i * 5 + 3];
//
//                mx_d += (double) dx / 4.0;
//                my_d -= (double) dy / 12.0;
//
//                if (mx_d < 0) {
//                    mx_d = 0;
//                }
//                if (mx_d >= con_width - 1) {
//                    mx_d = con_width - 2;
//                }
//
//                if (my_d < 0) {
//                    my_d = 0;
//                }
//                if (my_d >= con_height - 1) {
//                    my_d = con_height - 2;
//                }
//
//                mx = mx_d;
//                my = my_d;
//                puts2("\033[2J\033[1;1f");
//                printf("\033[%d;%dfX\033[%d;%df", my + 1, mx + 1, my + 1, mx + 1);
//            }
//            break;
//        }
//    }
//}

int main(int argc, char **argv) {
    fd_set fds;
    struct timeval tv;
    struct termios old_ts, ts;
    struct winsize winsz;
    int kb_fd = STDIN_FILENO, ms_fd;
    char buf[512];
    //ssize_t len;

    signal(SIGINT, signal_handle);

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsz) != 0) {
        perror("ioctl()");
        return -1;
    }
    printf("Terminal is %dx%d\n", winsz.ws_col, winsz.ws_row);
    con_width = winsz.ws_col;
    con_height = winsz.ws_row;
    if (tcgetattr(kb_fd, &old_ts) != 0) {
        perror("tcgetattr()");
        return -1;
    }
    memcpy(&ts, &old_ts, sizeof(struct termios));
    ts.c_iflag &= ~ICANON;
    ts.c_lflag = 0;
    if (tcsetattr(kb_fd, TCSANOW, &ts) != 0) {
        perror("tcsetattr()");
        return -1;
    }

    ms_fd = open("/dev/ps2aux", O_RDONLY, 0);

    while (running) {
        FD_ZERO(&fds);
        FD_SET(ms_fd, &fds);
        FD_SET(kb_fd, &fds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int res = select(ms_fd + 1, &fds, NULL, NULL, &tv);

        if (res < 0) {
            perror("select()");
            break;
        }

        if (res != 0) {
            if (FD_ISSET(ms_fd, &fds)) {
                read(ms_fd, buf, sizeof(buf));
                printf("Mouse\n");
                //ms_handle(buf, len);
            }
            if (FD_ISSET(kb_fd, &fds)) {
                read(kb_fd, buf, sizeof(buf));
                printf("Keyboard\n");
                //kb_handle(buf, len);
                if (buf[0] == 'q') {
                    break;
                }
            }
        } else {
            printf("Timeout\n");
        }
    }

    if (tcsetattr(kb_fd, TCSANOW, &old_ts) != 0) {
        perror("tcsetattr()");
        return -1;
    }
    printf("Stopped\n");

    return 0;
}
