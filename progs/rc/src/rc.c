#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
//#include <wait.h>

#define INITTAB         "/etc/inittab"
// Default runlevel
#define RUNLEVEL_1      (1 << 0)

enum rc_action {
    RC_ONCE,
    RC_BOOT
};

struct rc_rule {
    char id[4];
    uint32_t runlevels;
    enum rc_action action;

    char process[256];
    char *argv[16];
    int argc;

    struct rc_rule *prev, *next;
};

static void sig_handler(int signum) {
    if (signum == SIGTERM) {
        // TODO: reap children
        fprintf(stderr, "init received SIGTERM\n");
        return;
    }
}

static int load_inittab(struct rc_rule **_head) {
    struct rc_rule *head = NULL, *tail = NULL;
    FILE *fp;
    char buf[1024];
    char *r;
    char *e;

    // Parse and execute rules from inittab
    fp = fopen(INITTAB, "r");
    if (!fp) {
        perror(INITTAB);
        return -1;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        // Terminate with either a newline or a comment
        if (buf[0] && (r = strpbrk(buf, "\n\r#")) != NULL) {
            *r = 0;
        }

        // Skip leading whitespace
        r = buf;
        while (isspace(*r)) {
            ++r;
        }

        if (!*r) {
            continue;
        }

        // Parse the rule
        struct rc_rule *rule = malloc(sizeof(struct rc_rule));
        rule->runlevels = 0;

        // {id}:...
        e = strchr(r, ':');
        if (!e) {
            fprintf(stderr, "Syntax error: `%s`\n", r);
            goto error;
        }
        if (e - r >= 4 || e == r) {
            fprintf(stderr, "Rule ID must be 1-4 symbols: `%s`\n", r);
            goto error;
        }

        strncpy(rule->id, r, e - r);
        rule->id[e - r] = 0;

        // ...:{runlevels}:...

        r = e + 1;
        while (1) {
            if (*r == ':') {
                ++r;
                break;
            }

            if (!*r || !isdigit(*r)) {
                goto error;
            }

            switch (*r) {
            case '1':
                rule->runlevels |= RUNLEVEL_1;
                break;
            default:
                goto error;
            }

            ++r;
        }

        // ...:{action}:...
        e = strchr(r, ':');
        if (!e) {
            goto error;
        }

        *e = 0;
        if (!strcmp(r, "once")) {
            rule->action = RC_ONCE;
        } else {
            goto error;
        }
        r = e + 1;

        // ...:{process}
        strcpy(rule->process, r);

        // Split "process" string
        r = rule->process;
        while (1) {
            e = strchr(r, ' ');
            if (!e) {
                if (*r) {
                    rule->argv[rule->argc++] = r;
                }
                break;
            } else {
                *e = 0;
                rule->argv[rule->argc++] = r;
            }

            r = e + 1;
            while (isspace(*r)) {
                ++r;
            }
            if (!*r) {
                break;
            }
        }
        rule->argv[rule->argc] = NULL;

        if (tail) {
            tail->next = rule;
        } else {
            head = rule;
        }
        rule->next = NULL;
        rule->prev = tail;
        tail = rule;
    }

    *_head = head;

    fclose(fp);
    return 0;
error:
    fclose(fp);
    return -1;
}

static int rc_start(const struct rc_rule *rule) {
    int pid;

    if ((pid = fork()) < 0) {
        perror("fork()");
        return -1;
    }

    if (pid == 0) {
        exit(execve(rule->argv[0], rule->argv, environ));
    } else {
        // TODO: "wait" action
        return 0;
    }
}

int main(int argc, char **argv) {
    int runlevel = RUNLEVEL_1;
    signal(SIGTERM, sig_handler);

    struct rc_rule *head;
    if (load_inittab(&head) != 0) {
        return -1;
    }

    // Run all "boot" rules
    for (struct rc_rule *r = head; r; r = r->next) {
        if (r->action == RC_BOOT) {
            rc_start(r);
        }
    }

    // Run all "once" rules
    for (struct rc_rule *r = head; r; r = r->next) {
        if ((r->runlevels & runlevel) && r->action == RC_ONCE) {
            rc_start(r);
        }
    }

    while (1) {
        usleep(1000000);
    }
}
