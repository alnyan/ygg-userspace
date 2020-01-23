#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "builtin.h"
#include "config.h"
#include "cmd.h"

#define DEF_BUILTIN(b_name) \
    static int __bcmd_##b_name(const struct cmd_exec *cmd)
#define DECL_BUILTIN(b_name) \
    { .name = #b_name, .exec = __bcmd_##b_name }

struct sh_builtin {
    const char *name;
    int (*exec) (const struct cmd_exec *cmd);
};

static struct sh_builtin __builtins[];

////

DEF_BUILTIN(exit) {
    if (cmd->argc > 1) {
        exit(atoi(cmd->args[1]));
    }
    exit(0);
}

DEF_BUILTIN(cd) {
    if (cmd->argc != 2) {
        printf("usage: cd <path>\n");
        return -1;
    }
    return chdir(cmd->args[1]);
}

DEF_BUILTIN(cat) {
    if (cmd->argc < 2) {
        printf("usage: cat <filename> ...\n");
        return -1;
    }
    char buf[4096];
    ssize_t bread;

    for (int i = 1; i < cmd->argc; ++i) {
        int fd = open(cmd->args[i], O_RDONLY, 0);
        if (fd < 0) {
            perror(cmd->args[i]);
            continue;
        }

        while ((bread = read(fd, buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, bread);
        }

        if (bread < 0) {
            perror(cmd->args[i]);
        }

        close(fd);
    }
    return 0;
}

DEF_BUILTIN(exec) {
    //struct cmd_exec new_cmd;
    if (cmd->argc < 2) {
        printf("usage: exec <command> ...\n");
    }

    char path_path[256];
    int res;

    if (cmd->args[1][0] == '.' || cmd->args[1][0] == '/') {
        if ((res = access(cmd->args[1], X_OK)) != 0) {
            perror(cmd->args[1]);
            return res;
        }

        strcpy(path_path, cmd->args[1]);
        exit(execve(path_path, (const char **) &cmd->args[1], NULL));
    }

    snprintf(path_path, sizeof(path_path), "%s/%s", PATH, cmd->args[1]);
    if ((res = access(path_path, X_OK)) == 0) {
        exit(execve(path_path, (const char **) &cmd->args[1], NULL));
    }

    return -1;
}

DEF_BUILTIN(clear) {
    puts2("\033[2J\033[1;1f");
    return 0;
}

DEF_BUILTIN(echo) {
    for (int i = 1; i < cmd->argc; ++i) {
        printf("%s ", cmd->args[i]);
    }
    printf("\n");
    return 0;
}

DEF_BUILTIN(into) {
    if (cmd->argc < 3) {
        printf("usage: into <filename> <command> ...\n");
        return -1;
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork()");
        return -1;
    }

    if (pid == 0) {
        close(STDIN_FILENO);
        int fd = open(cmd->args[1], O_RDONLY, 0);
        if (fd < 0) {
            perror(cmd->args[1]);
            return -1;
        }

        exit(execve(cmd->args[2], (const char *const *) &cmd->args[2], NULL));
    } else {
        int st;
        waitpid(pid, &st);
        return 0;
    }
}

// TODO: support usernames (getpwnam_r)
DEF_BUILTIN(setid) {
    if (cmd->argc == 2) {
        // Assume gid == uid
        if (setuid(atoi(cmd->args[1])) != 0) {
            return -1;
        }
        if (setgid(atoi(cmd->args[1])) != 0) {
            return -1;
        }
        return 0;
    }
    if (cmd->argc != 3) {
        printf("usage: setid <uid> <gid>\n");
        return -1;
    }

    if (setuid(atoi(cmd->args[1])) != 0) {
        return -1;
    }
    if (setgid(atoi(cmd->args[2])) != 0) {
        return -1;
    }
    return 0;
}

DEF_BUILTIN(builtins) {
    for (size_t i = 0; __builtins[i].name; ++i) {
        printf("%s ", __builtins[i].name);
    }
    printf("\n");
    return 0;
}

////

static struct sh_builtin __builtins[] = {
    DECL_BUILTIN(builtins),
    DECL_BUILTIN(cat),
    DECL_BUILTIN(cd),
    DECL_BUILTIN(clear),
    DECL_BUILTIN(echo),
    DECL_BUILTIN(exec),
    DECL_BUILTIN(exit),
    DECL_BUILTIN(into),
    DECL_BUILTIN(setid),
    {NULL}
};

int builtin_exec(const struct cmd_exec *cmd, int *cmd_res) {
    for (size_t i = 0; i < sizeof(__builtins) / sizeof(__builtins[0]); ++i) {
        if (!__builtins[i].name) {
            return -1;
        }
        if (!strcmp(__builtins[i].name, cmd->args[0])) {
            *cmd_res = __builtins[i].exec(cmd);
            return 0;
        }
    }

    return -1;
}
