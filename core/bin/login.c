#include <sys/ioctl.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>

struct spwd {
    char *sp_namp;
    char *sp_pwdp;
};

static int attempt = 0;
static char line_buf[64];

static ssize_t getline(char *buf, size_t lim, char pwchr) {
    size_t c = 0;
    char chr;

    while (1) {
        if (c == lim) {
            return -1;
        }

        if (read(STDIN_FILENO, &chr, 1) != 1) {
            return -1;
        }

        switch (chr) {
        case '\n':
            putchar('\n');
            buf[c] = 0;
            return c;
        case '\b':
            if (c) {
                buf[--c] = 0;
                puts2("\033[D \033[D");
            }
            break;
        default:
            if (chr >= ' ' && chr < 255) {
                if (pwchr) {
                    putchar(pwchr);
                } else {
                    putchar(chr);
                }
                buf[c++] = chr;
            }
            break;
        }
    }
}

static int getspnam_r(const char *name, struct spwd *sp, char *buf, size_t buf_size) {
    int fd = open("/etc/shadow", O_RDONLY, 0);
    if (fd < 0) {
        return fd;
    }

    while (gets_safe(fd, buf, buf_size) > 0) {
        char *p0 = strchr(buf, ':');
        if (!p0) {
            break;
        }
        char *p1 = strchr(p0 + 1, ':');
        if (!p1) {
            break;
        }

        *p0 = 0;
        *p1 = 0;

        if (strcmp(buf, name)) {
            continue;
        }

        close(fd);
        sp->sp_namp = buf;
        sp->sp_pwdp = p0 + 1;
        return 0;
    }

    close(fd);
    errno = ENOENT;
    return -1;
}

static int loginuid(uid_t uid, gid_t gid, const char *sh) {
    int sh_pid = fork();

    if (sh_pid < 0) {
        return -1;
    }

    if (sh_pid == 0) {
        if (setuid(uid) || setgid(gid)) {
            perror("login");
            exit(-1);
        }

        setpgid(0, 0);
        int pid = getpid();
        ioctl(STDIN_FILENO, TIOCSPGRP, &pid);

        const char *argp[] = { sh, NULL };
        exit(execve(sh, argp, NULL));
    } else {
        int st;
        waitpid(sh_pid, &st);
        // Regain control of the terminal
        int pgid = getpgid(0);
        ioctl(STDIN_FILENO, TIOCSPGRP, &pgid);
        return 0;
    }
}

static int loginas(const char *name) {
    char pwbuf[512];
    struct passwd pwd;
    struct passwd *res;
    const char *shell;

    if (getpwnam_r(name, &pwd, pwbuf, sizeof(pwbuf), &res) != 0) {
        perror("getpwnam_r()");
        return -1;
    }

    shell = res->pw_shell;

    if (access(shell, X_OK) != 0) {
        perror(shell);
        printf("Using /bin/sh instead\n");
        shell = "/bin/sh";
    }

    return loginuid(res->pw_uid, res->pw_gid, shell);
}

static void signal_handler(int signum) {
    // TODO: syscalls interruptible by singals
}

int main(int argc, char **argv) {
    // Arguments are ignored
    if (getuid() != 0) {
        printf("login must be run as root\n");
        return -1;
    }

    signal(SIGINT, signal_handler);

    char spnam_buf[128];
    struct spwd sp;

    while (access("/etc/shadow", R_OK) != 0) {
        perror("/etc/shadow");
        // Login as passwordless root?
        if (loginuid(0, 0, "/bin/sh") != 0) {
            return -1;
        }
        // Maybe after this session /etc/shadow becomes available
    }

    while (1) {
        if (attempt == 3) {
            puts2("\033[2J\033[1;1f");
            attempt = 0;
        }

        printf("login: ");
        if (getline(line_buf, sizeof(line_buf), 0) < 0) {
            break;
        }

        if (getspnam_r(line_buf, &sp, spnam_buf, sizeof(spnam_buf)) != 0) {
            perror("getspnam_r()");
            ++attempt;
            continue;
        }

        if (sp.sp_pwdp[0] != 0) {
            printf("password: ");
            if (getline(line_buf, sizeof(line_buf), '*') < 0) {
                ++attempt;
                continue;
            }
        } else {
            line_buf[0] = 0;
        }

        // No hashing yet, so just compare passwords
        if (!strcmp(line_buf, sp.sp_pwdp)) {
            loginas(sp.sp_namp);
            // After returning from shell, go here
            attempt = 3;
            continue;
        } else {
            printf("Login failed\n");
            ++attempt;
        }
    }

    return -1;
}
