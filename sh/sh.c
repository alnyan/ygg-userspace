#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <pwd.h>

#include "readline.h"
#include "config.h"
#include "cmd.h"

// Data needed for prompt
static char p_hostname[64];
static char p_username[64];
static char p_cwd[256];
static uid_t p_uid;
//static gid_t p_gid;

static void update_prompt(void) {
    struct utsname _u;
    struct passwd _p;
    struct passwd *p;
    char pwbuf[512];

    p_uid = getuid();

    if (getcwd(p_cwd, sizeof(p_cwd)) == NULL) {
        perror("getcwd()");
        p_cwd[0] = 0;
    }

    if (uname(&_u) != 0) {
        perror("uname()");
        p_hostname[0] = 0;
    } else {
        strcpy(p_hostname, _u.nodename);
    }

    if (getpwuid_r(p_uid, &_p, pwbuf, sizeof(pwbuf), &p) != 0) {
        perror("getpwuid_r()");
        p_username[0] = 0;
    } else {
        strcpy(p_username, p->pw_name);
    }
}

static void display_prompt(void) {
    const char *p = PS1;

    while (*p) {
        if (*p == '%') {
            switch (*++p) {
            case '#':
                putchar(p_uid == 0 ? '#' : '$');
                break;
            case 'h':
                puts2(p_hostname);
                break;
            case 'u':
                puts2(p_username);
                break;
            case 'd':
                puts2(p_cwd);
                break;
            default:
                putchar('%');
                putchar(*p);
                break;
            }
        } else {
            putchar(*p);
        }

        ++p;
    }
}

static void signal_handle(int signum) {
    switch (signum) {
    case SIGINT:
        printf("\n");
        update_prompt();
        display_prompt();
        break;
    default:
        printf("\nUnhandled signal: %d\n", signum);
        break;
    }
}

int main(int argc, char **argv) {
    int fd = STDIN_FILENO;
    char linebuf[256];
    int res;

    signal(SIGINT, signal_handle);

    if (argc == 2) {
        fd = open(argv[1], O_RDONLY, 0);

        if (fd < 0) {
            perror(argv[1]);
            return -1;
        }
    } else if (argc != 1) {
        printf("usage: sh [filename]\n");
        return -1;
    }

    if (!isatty(fd)) {
        while (1) {
            if (gets_safe(fd, linebuf, sizeof(linebuf)) < 0) {
                break;
            }

            eval(linebuf);
        }

        if (fd != STDIN_FILENO) {
            close(fd);
        }

        return 0;
    }

    while (1) {
        update_prompt();
        display_prompt();

        if (readline(linebuf, sizeof(linebuf)) < 0) {
            break;
        }

        if ((res = eval(linebuf)) != 0) {
            printf(COLOR_RED "Status: %d" COLOR_RESET "\n", res);
        }
    }

    return 0;
}
