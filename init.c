#include <ygg/netctl.h>
// TODO:
//#include <sys/mount.h>
#include <sys/wait.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

// TODO
int mount(const char *source, const char *target,
          const char *fs, unsigned long flags,
          void *data);

// The only thing init does now
static int start_login(void) {
    const char *login = "/bin/login";
    const char *const argp[] = {
        login, NULL
    };

    int pid = fork();
    int res;

    switch (pid) {
    case 0:
        return execve(login, (char *const *) argp, environ);
    case -1:
        fprintf(stderr, "Init program failed\n");
        return -1;
    default:
        waitpid(pid, &res, 0);
        return 0;
    }

}

int main(int argc, char **argv) {
    if (getpid() != 1) {
        printf("Won't work if PID is not 1\n");
        return -1;
    }

    mount(NULL, "/dev", "devfs", 0, NULL);
    mount(NULL, "/sys", "sysfs", 0, NULL);
    mount("/dev/sda", "/mnt", NULL, 0, NULL);

    uint32_t inaddr = 0x0A000001;
    netctl("eth0", NETCTL_SET_INADDR, &inaddr);

    return start_login();
}
