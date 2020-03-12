#include <sys/termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "builtin.h"
#include "config.h"
#include "cmd.h"

static int make_cmd(char *input, struct cmd_exec *ex) {
    char *p = strchr(input, ' ');
    char *e;

    if (!p) {
        ex->args[0] = input;
        ex->args[1] = NULL;
        ex->argc = 1;
        return 0;
    }

    *p++ = 0;
    ex->args[0] = input;
    ex->argc = 1;

    while (1) {
        while (isspace(*p)) {
            ++p;
        }
        if (!*p) {
            break;
        }

        e = strchr(p, ' ');

        if (!e) {
            ex->args[ex->argc++] = p;
            break;
        } else {
            *e++ = 0;
            ex->args[ex->argc++] = p;
            p = e;
        }
    }
    ex->args[ex->argc] = NULL;

    return 0;
}

//

static int cmd_spawn(const char *path, const struct cmd_exec *cmd, int *cmd_res) {
    int pid;

    if ((pid = fork()) < 0) {
        perror("fork()");
        return -1;
    }

    if (pid == 0) {
        pid = getpid();
        ioctl(STDIN_FILENO, TIOCSPGRP, &pid);
        setpgid(0, 0);
        exit(execve(path, (char *const *) cmd->args, NULL));
    } else {
        if (waitpid(pid, cmd_res) != 0) {
            perror("waitpid()");
        }

        // Regain control of foreground group
        pid = getpgid(0);
        ioctl(STDIN_FILENO, TIOCSPGRP, &pid);

        return 0;
    }
}

static int cmd_exec_binary(const struct cmd_exec *cmd, int *cmd_res) {
    char path_path[256];
    int res;

    if (cmd->args[0][0] == '.' || cmd->args[0][0] == '/') {
        if ((res = access(cmd->args[0], X_OK)) != 0) {
            perror(cmd->args[0]);
            return res;
        }

        strcpy(path_path, cmd->args[0]);
        return cmd_spawn(path_path, cmd, cmd_res);
    }

    snprintf(path_path, sizeof(path_path), "%s/%s", PATH, cmd->args[0]);
    if ((res = access(path_path, X_OK)) == 0) {
        return cmd_spawn(path_path, cmd, cmd_res);
    }
    return -1;
}

static int cmd_exec(const struct cmd_exec *cmd) {
    int res, cmd_res;

    if ((res = builtin_exec(cmd, &cmd_res)) == 0) {
        return cmd_res;
    }

    if ((res = cmd_exec_binary(cmd, &cmd_res)) == 0) {
        return cmd_res;
    }

    printf("sh: command not found: %s\n", cmd->args[0]);
    return -1;
}

int eval(char *str) {
    struct cmd_exec cmd;
    char *p;

    while (isspace(*str)) {
        ++str;
    }

    if ((p = strchr(str, '#'))) {
        *p = 0;
    }

    if (!*str) {
        return 0;
    }

    if (make_cmd(str, &cmd) != 0) {
        abort();
    }

    return cmd_exec(&cmd);
}
