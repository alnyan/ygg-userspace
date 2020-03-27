#include <ygg/video.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

static int running = 1;

static void signal_handler(int signum) {
    running = 0;
}

int main(int argc, char **argv) {
    struct ioc_vmode mode;
    int fd = open("/dev/fb0", O_RDONLY, 0);
    if (fd < 0) {
        perror("open()");
        return -1;
    }

    if (ioctl(fd, IOC_GETVMODE, &mode) != 0) {
        perror("ioctl()");
        goto end;
    }

    signal(SIGINT, signal_handler);

    printf("Video mode: %dx%d\n", mode.width, mode.height);

    size_t size = mode.width * mode.height * 4;
    size = (size + 0xFFF) & ~0xFFF;
    printf("%u bytes\n", size);

    printf("You can exit by pressing ^C\n");
    printf("Starting video in 3 secs\n");
    usleep(3000000);

    int no = 0;
    if (ioctl(fd, IOC_FBCON, &no) != 0) {
        perror("ioctl()");
        goto  end;
    }

    uint32_t *data = mmap(NULL, size, 0, MAP_PRIVATE, fd, 0);
    assert(data);

    struct timeval tv;
    int c = 0;
    int s = 0;

    while (running) {
        usleep(10000);
        gettimeofday(&tv, NULL);
        if (s == 0) {
            c += 2;
        } else {
            c -= 2;
        }
        if (c < 0) {
            s = 0;
            c = 0;
        } else if (c >= 256) {
            s = 1;
            c = 255;
        }
        memset(data, c, size);
    }

    munmap(data, size);
    no = 1;
    if (ioctl(fd, IOC_FBCON, &no) != 0) {
        perror("ioctl()");
        goto  end;
    }
    puts2("\033[2J\033[1;1f");
    printf("Goodbye\n");
end:
    close(fd);

    return 0;
}
