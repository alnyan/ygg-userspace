#include <stdio.h>
#include <time.h>

int main(int argc, char **argv) {
    // Ignore arguments

    struct timeval tv0;
    gettimeofday(&tv0, NULL);
    struct tm tm0;
    gmtime_r(&tv0.tv_sec, &tm0);
    printf("%04u-%02u-%02u %02u:%02u:%02u\n",
            tm0.tm_year, tm0.tm_mon, tm0.tm_mday,
            tm0.tm_hour, tm0.tm_min, tm0.tm_sec);

    return 0;
}
