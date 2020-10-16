#include <sys/debug.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <err.h>

#define INITTAB         "/etc/inittab"
// Default runlevel
#define RUNLEVEL_1      (1 << 0)

enum rc_action {
    RC_ONCE,
    RC_BOOT,
    RC_WAIT
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

static void rc_inittab_error(int line, const char *msg) {
    ygg_debug_trace("rc: error on line %d\n", line);
    ygg_debug_trace("rc:   %s\n", msg);
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
        warn("fopen(%s)", INITTAB);
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
        if (e - r > 4 || e == r) {
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
            rc_inittab_error(0, r);
            goto error;
        }

        *e = 0;
        if (!strcmp(r, "once")) {
            rule->action = RC_ONCE;
        } else if (!strcmp(r, "wait")) {
            rule->action = RC_WAIT;
        } else {
            rc_inittab_error(0, r);
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
    ygg_debug_trace("rc: start `%s`\n", rule->argv[0]);

    if ((pid = fork()) < 0) {
        warn("fork()");
        return -1;
    }

    if (pid == 0) {
        exit(execve(rule->argv[0], rule->argv, environ));
    } else {
        return pid;
    }
}

static int rc_enter_runlevel(const struct rc_rule *head, int runlevel) {
    // Run "once" and "wait" rules
    int n_failed = 0;
    for (const struct rc_rule *r = head; r; r = r->next) {
        if ((r->runlevels & runlevel) && (r->action == RC_ONCE || r->action == RC_WAIT)) {
            int pid = rc_start(r);
            if (pid < 0) {
                ygg_debug_trace("rc: rule start failed: fork() failed\n");
                ++n_failed;
            } else {
                if (r->action == RC_WAIT) {
                    int status;
                    int r = waitpid(pid, &status, 0);
                    if (r < 0) {
                        ygg_debug_trace("rc: rule waitpid() failed\n");
                    }
                }
            }
        }
    }
    return n_failed;
}

int main(int argc, char **argv) {
    int runlevel = RUNLEVEL_1;

    struct rc_rule *head;
    if (load_inittab(&head) != 0) {
        ygg_debug_trace("rc: failed to load inittab\n");
        return -1;
    }
    ygg_debug_trace("rc: inittab loaded\n");

    // Run all "boot" rules
    ygg_debug_trace("rc: running `boot` ruleset\n");
    for (struct rc_rule *r = head; r; r = r->next) {
        if (r->action == RC_BOOT) {
            rc_start(r);
        }
    }

    // Enter target runlevel
    rc_enter_runlevel(head, runlevel);

    while (1) {
        int status;
        waitpid(-1, &status, 0);
    }
}
