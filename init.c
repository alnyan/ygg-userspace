#include <sys/mount.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

// The only thing init does now
static int start_login(void) {
    const char *login = "/bin/login";
    const char *const argp[] = {
        login, NULL
    };
    // No fork needed, I guess
    return execve(login, (char *const *) argp, NULL);
}

static void t0(void) {
    //uint32_t *r = mmap(NULL, 0x1000, 0, 0, -1, 0);
    //r[0] = 0x1234;
    //usleep(10000);
}

static void t1(void) {
    //uint32_t *r = mmap(NULL, 0x1000, 0, 0, -1, 0);
    //usleep(1000000);
    //printf("%04x\n", r[0]);
}

int main(int argc, char **argv) {
    (void) start_login;
    int c0, c1, status;

    printf("Start thread 1\n");
    c0 = fork();

    if (c0 == 0) {
        t0();
        return 0;
    }

    printf("Start thread 2\n");
    c1 = fork();

    if (c1 == 0) {
        t1();
        return 0;
    }

    waitpid(c1, &status);
    waitpid(c0, &status);

    printf("Children dead\n");

    while (1) {
        usleep(1000000);
    }

    return 0;
    //if (getpid() != 1) {
    //    printf("Won't work if PID is not 1\n");
    //    return -1;
    //}

    //mount(NULL, "/dev", "devfs", 0, NULL);
    //mount(NULL, "/sys", "sysfs", 0, NULL);
    //mount("/dev/sda1", "/mnt", NULL, 0, NULL);

    //return start_login();
}
