#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "builtin.h"
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

DEF_BUILTIN(clear) {
    puts2("\033[2J\033[1;1f");
    return 0;
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
    DECL_BUILTIN(cd),
    DECL_BUILTIN(clear),
    DECL_BUILTIN(exit),
    DECL_BUILTIN(setid),
    {NULL}
};


int builtin_exec(const struct cmd_exec *cmd, int *cmd_res) {
    for (size_t i = 0; i < sizeof(__builtins) / sizeof(__builtins[0]); ++i) {
        if (!strcmp(__builtins[i].name, cmd->args[0])) {
            *cmd_res = __builtins[i].exec(cmd);
            return 0;
        }
    }

    return -1;
}
