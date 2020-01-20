#include <stdio.h>

int main(int argc, char **argv) {
    if (getuid() != 0) {
        printf("su binary must have setuid bit on\n");
        return -1;
    }

    if (setgid(0) != 0) {
        perror("setgid()");
        return -1;
    }

    const char *shell = "/bin/sh";
    const char *argp[] = {
        shell, NULL
    };
    return execve(shell, argp, NULL);
}
