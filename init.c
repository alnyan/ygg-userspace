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

int main(int argc, char **argv) {
    if (getpid() != 1) {
        printf("Won't work if PID is not 1\n");
        return -1;
    }

    mount(NULL, "/dev", "devfs", 0, NULL);
    mount(NULL, "/sys", "sysfs", 0, NULL);
    mount("/dev/sda1", "/mnt", NULL, 0, NULL);

    return start_login();
}
