#include <ygg/video.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <termios.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static int running = 1;
static double mx_d = 40, my_d = 12;

static uint8_t cursor[8] = {
    0b11111111,
    0b11111110,
    0b11111100,
    0b11111000,
    0b11110000,
    0b11100000,
    0b11000000,
    0b10000000,
};

static uint32_t *vmem;
static size_t vsize;

static void signal_handler(int signum) {
    running = 0;
}

static void ms_handle(char *buf, size_t len) {
    for (size_t i = 0; i < len / 5; ++i) {
        char type = buf[i * 5 + 0];

        switch (type) {
        case 'd': {
                int16_t dx = *(int16_t *) &buf[i * 5 + 1];
                int16_t dy = *(int16_t *) &buf[i * 5 + 3];

                mx_d += (double) dx / 4.0;
                my_d -= (double) dy / 6.0;

                if (mx_d < 0) {
                    mx_d = 0;
                }
                if (mx_d >= mode.width - 1) {
                    mx_d = mode.width - 2;
                }

                if (my_d < 0) {
                    my_d = 0;
                }
                if (my_d >= mode.height - 1) {
                    my_d = mode.height - 2;
                }
            }
            break;
        }
    }
}

static void rect(int x0, int y0, int x1, int y1, uint32_t col) {
    for (int x = MAX(x0, 0); x <= MIN(x1, mode.width - 1); ++x) {
        for (int y = MAX(y0, 0); y <= MIN(y1, mode.height - 1); ++y) {
            vmem[y * mode.width + x] = col;
        }
    }
}

static void draw_cursor(int x, int y, uint32_t col) {
    for (int i = 0; i < 8; ++i) {
        if (i + x >= mode.width) {
            break;
        }
        for (int j = 0; j < 8; ++j) {
            if (j + y >= mode.height) {
                break;
            }

            if (!(cursor[j] & (1 << (7 - i)))) {
                continue;
            }

            vmem[(j + y) * mode.width + x + i] = col;
        }
    }
}

static void render(void) {
    static double px, py;

    rect(px, py, px + 8, py + 8, 0);

    px = mx_d;
    py = my_d;

    draw_cursor(px, py, 0xFF0000);
}

int main(int argc, char **argv) {
    int fd = open("/dev/fb0", O_RDONLY, 0);
    int mouse_fd = open("/dev/ps2aux", O_RDONLY, 0);
    struct termios tc, tc_old;
    struct timeval tv;
    fd_set fds;
    char buf[512];
    ssize_t len;
    int no = 0;

    tc.c_iflag = 0;
    tc.c_lflag = 0;
    tc.c_oflag = 0;

    if (tcgetattr(STDIN_FILENO, &tc_old) != 0) {
        perror("tcgetattr()");
        goto end;
    }

    if (fd < 0 || mouse_fd < 0) {
        perror("open()");
        return -1;
    }

    if (ioctl(fd, IOC_GETVMODE, &mode) != 0) {
        perror("ioctl()");
        goto end;
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &tc) != 0) {
        perror("tcsetattr()");
        goto end;
    }

    signal(SIGINT, signal_handler);

    printf("Video mode: %dx%d\n", mode.width, mode.height);

    vsize = mode.width * mode.height * 4;
    vsize = (vsize + 0xFFF) & ~0xFFF;
    printf("%u bytes\n", vsize);

    printf("You can exit by pressing ^C\n");
    printf("Starting video in 1 sec\n");
    usleep(1000000);

    if (!running) {
        // Interrupt happened
        close(mouse_fd);
        close(fd);

        if (tcsetattr(STDIN_FILENO, TCSANOW, &tc_old) != 0) {
            perror("tcsetattr()");
            goto end;
        }

        puts2("\033[2J\033[1;1f");
        printf("Goodbye\n");

        return 0;
    }

    if (ioctl(fd, IOC_FBCON, &no) != 0) {
        perror("ioctl()");
        goto  end;
    }

    vmem = mmap(NULL, vsize, 0, MAP_PRIVATE, fd, 0);
    assert(vmem);

    memset(vmem, 0, vsize);

    while (running) {
        tv.tv_sec = 0;
        tv.tv_usec = 10000;
        FD_ZERO(&fds);
        FD_SET(mouse_fd, &fds);
        FD_SET(STDIN_FILENO, &fds);

        int res = select(mouse_fd + 1, &fds, NULL, NULL, &tv);

        if (res < 0) {
            no = 1;
            if (ioctl(fd, IOC_FBCON, &no) != 0) {
                perror("ioctl()");
                goto  end;
            }

            printf("select() failed\n");

            break;
        }

        if (res > 0) {
            if (FD_ISSET(mouse_fd, &fds)) {
                len = read(mouse_fd, buf, sizeof(buf));
                ms_handle(buf, len);
            }
            if (FD_ISSET(STDIN_FILENO, &fds)) {
                read(STDIN_FILENO, buf, sizeof(buf));

                if (buf[0] == 'q') {
                    break;
                }
            }
        }

        render();
    }

    munmap(vmem, vsize);
    no = 1;
    if (ioctl(fd, IOC_FBCON, &no) != 0) {
        perror("ioctl()");
        goto  end;
    }
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tc_old) != 0) {
        perror("tcsetattr()");
        goto end;
    }

    puts2("\033[2J\033[1;1f");
    printf("Goodbye\n");
end:
    close(fd);

    return 0;
}
