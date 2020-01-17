#include <unistd.h>
#include <stdio.h>

// The only thing init does now
static int start_login(void) {
    const char *login = "/bin/login";
    const char *const argp[] = {
        login, NULL
    };
    // No fork needed, I guess
    return execve(login, argp, NULL);
}

int main(int argc, char **argv) {
    if (getpid() != 1) {
        printf("Won't work if PID is not 1\n");
        return -1;
    }
    return start_login();
}
