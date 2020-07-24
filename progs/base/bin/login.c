#include <sys/ioctl.h>
#include <sys/wait.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>

struct spwd {
    char *sp_namp;
    char *sp_pwdp;
};

static int attempt = 0;
static char line_buf[64];

static ssize_t login_getline(char *buf, size_t lim, int echo) {
    ssize_t len;
    struct termios old_tc;
    struct termios tc;

    if (!echo) {
        // Disable echo
        if (tcgetattr(STDIN_FILENO, &old_tc)) {
            perror("tcgetattr()");
            return -1;
        }
        memcpy(&tc, &old_tc, sizeof(struct termios));
        // This won't show typed characters, but newlines will still be visible
        tc.c_lflag &= ~(ECHO | ECHOE);
        if (tcsetattr(STDIN_FILENO, TCSANOW, &tc)) {
            perror("tcsetattr()");
            return -1;
        }
    }

    len = read(STDIN_FILENO, buf, lim);

    if (!echo) {
        // Restore terminal to sane state
        if (tcsetattr(STDIN_FILENO, TCSANOW, &old_tc)) {
            perror("tcsetattr()");
            return -1;
        }
    }

    if (len <= 0) {
        return -1;
    }

    // Remove trailing newline
    if (buf[len - 1] == '\n') {
        buf[len - 1] = 0;
    }

    return len;
}

static int getspnam_r(const char *name, struct spwd *sp, char *buf, size_t buf_size) {
    FILE *fp = fopen("/etc/shadow", "r");
    if (!fp) {
        return -1;
    }

    while (fgets(buf, buf_size, fp) != NULL) {
        char *p0;
        if (*buf && (p0 = strchr(buf, '\n')) != NULL) {
            *p0 = 0;
        }

        p0 = strchr(buf, ':');
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

        fclose(fp);
        sp->sp_namp = buf;
        sp->sp_pwdp = p0 + 1;
        return 0;
    }

    fclose(fp);
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
            _exit(-1);
        }

        setpgid(0, 0);
        int pid = getpid();
        ioctl(STDIN_FILENO, TIOCSPGRP, &pid);

        const char *argp[] = { sh, NULL };
        _exit(execve(sh, (char *const *) argp, environ));
    } else {
        int st;
        waitpid(sh_pid, &st, 0);
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
            printf("\033[2J\033[1;1f");
            fflush(stdout);
            attempt = 0;
        }

        printf("login: ");
        fflush(stdout);
        if (login_getline(line_buf, sizeof(line_buf), 1) < 0) {
            break;
        }

        if (getspnam_r(line_buf, &sp, spnam_buf, sizeof(spnam_buf)) != 0) {
            perror("getspnam_r()");
            ++attempt;
            continue;
        }

        if (sp.sp_pwdp[0] != 0) {
            printf("password: ");
            fflush(stdout);
            if (login_getline(line_buf, sizeof(line_buf), 0) < 0) {
                ++attempt;
                continue;
            }
        } else {
            line_buf[0] = 0;
        }

        // No hashing yet, so just compare passwords
        if (!strcmp(line_buf, sp.sp_pwdp) && loginas(sp.sp_namp) == 0) {
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
