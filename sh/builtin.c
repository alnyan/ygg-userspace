#include <sys/fcntl.h>
#include <sys/stat.h>
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

DEF_BUILTIN(chmod) {
    mode_t mode = 0;
    const char *p;
    if (cmd->argc != 3) {
        printf("usage: chmod <mode> <filename>\n");
        return -1;
    }
    p = cmd->args[1];
    while (*p) {
        if (*p > '7' || *p < '0') {
            printf("Invalid mode: %s\n", cmd->args[1]);
            return -1;
        }
        mode <<= 3;
        mode |= *p - '0';
        ++p;
    }

    if (chmod(cmd->args[2], mode)) {
        perror(cmd->args[2]);
        return -1;
    }

    return 0;
}

DEF_BUILTIN(stat) {
    struct stat st;
    if (cmd->argc != 2) {
        printf("usage: stat <filename>\n");
        return -1;
    }

    if (stat(cmd->args[1], &st) != 0) {
        return -1;
    }

    printf("device  %u\n", st.st_dev);
    printf("inode   %u\n", st.st_ino);
    printf("mode    %u\n", st.st_mode);
    printf("nlink   %u\n", st.st_nlink);
    printf("uid     %u\n", st.st_uid);
    printf("gid     %u\n", st.st_gid);
    printf("rdev    %u\n", st.st_rdev);
    printf("size    %u\n", st.st_size);
    printf("atime   %u\n", st.st_atime);
    printf("mtime   %u\n", st.st_mtime);
    printf("ctime   %u\n", st.st_ctime);
    printf("blksize %u\n", st.st_blksize);
    printf("blocks  %u\n", st.st_blocks);

    return 0;
}

DEF_BUILTIN(sync) {
    sync();
    return 0;
}

DEF_BUILTIN(touch) {
    int fd;
    if (cmd->argc != 2) {
        printf("usage: touch <filename>\n");
        return -1;
    }

    if ((fd = open(cmd->args[1], O_WRONLY | O_CREAT, 0755)) < 0) {
        perror(cmd->args[1]);
        return -1;
    }

    close(fd);

    return 0;
}

DEF_BUILTIN(chown) {
    uid_t uid;
    gid_t gid;
    const char *p;

    if (cmd->argc != 3) {
        printf("usage: chown <uid>:<gid> <filename>\n");
        return -1;
    }

    if (!(p = strchr(cmd->args[1], ':'))) {
        printf("Invalid UID:GID pair: %s\n", cmd->args[1]);
        return -1;
    }
    uid = atoi(cmd->args[1]);
    gid = atoi(p + 1);

    if (chown(cmd->args[2], uid, gid)) {
        perror(cmd->args[2]);
        return -1;
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
        exit(execve(path_path, (char *const *) &cmd->args[1], NULL));
    }

    snprintf(path_path, sizeof(path_path), "%s/%s", PATH, cmd->args[1]);
    if ((res = access(path_path, X_OK)) == 0) {
        exit(execve(path_path, (char *const *) &cmd->args[1], NULL));
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
    DECL_BUILTIN(chmod),
    DECL_BUILTIN(chown),
    DECL_BUILTIN(clear),
    DECL_BUILTIN(echo),
    DECL_BUILTIN(exec),
    DECL_BUILTIN(exit),
    DECL_BUILTIN(setid),
    DECL_BUILTIN(stat),
    DECL_BUILTIN(sync),
    DECL_BUILTIN(touch),
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
