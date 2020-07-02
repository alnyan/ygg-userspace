#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {
    struct timeval tv0;
    char buf[256];
    gettimeofday(&tv0, NULL);
    struct tm tm0;
    gmtime_r(&tv0.tv_sec, &tm0);
    strftime(buf, sizeof(buf), "%c\n", &tm0);
    write(STDOUT_FILENO, buf, strlen(buf));

    return 0;
}
