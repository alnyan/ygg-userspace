#include <sys/fcntl.h>
#include <sys/reboot.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

// Cursor helpers

#define curs_save() \
    write(STDOUT_FILENO, "\033[s", 3)
#define curs_unsave() \
    write(STDOUT_FILENO, "\033[u", 3)
#define curs_set(row, col) \
    printf("\033[%d;%df", row, col)
#define clear() \
    write(STDOUT_FILENO, "\033[2J", 4)

#define BOX_ANGLE_UL    "\332"
#define BOX_ANGLE_UR    "\277"
#define BOX_ANGLE_LL    "\300"
#define BOX_ANGLE_LR    "\331"
#define BOX_HOR         "\304"
#define BOX_VERT        "\263"

//

struct builtin {
    const char *name;
    const char *desc;
    int (*run) (const char *arg);
};

static int atoi(const char *in) {
    int r = 0;
    char c;
    while ((c = *in)) {
        if (c > '9' || c < '0') {
            break;
        }

        r *= 10;
        r += c - '0';

        ++in;
    }
    return r;
}

static int from_oct(int *res, const char *in) {
    int r = 0;
    while (*in) {
        if (*in < '0' || *in > '7') {
            return -1;
        }
        r <<= 3;
        r |= *in - '0';
        ++in;
    }
    *res = r;
    return 0;
}

static int readline(char *buf, size_t lim) {
    int len = 0;
    int cnt;
    char c;

    while (1) {
        if ((cnt = read(STDIN_FILENO, &c, 1)) != 1) {
            return -1;
        }

        if (len == lim) {
            printf("Input line is too long\n");
            return -1;
        }

        if (c == '\n') {
            putchar(c);
            buf[len] = 0;
            break;
        } else if (c == '\b') {
            if (len) {
                buf[--len] = 0;
                printf("\033[D \033[D");
            }
        } else if (c >= ' ') {
            putchar(c);
            buf[len++] = c;
        }
    }

    return len;
}

static int b_cd(const char *path);
static int b_pwd(const char *_);
static int b_cat(const char *path);
static int b_sleep(const char *arg);
static int b_help(const char *arg);
static int b_clear(const char *arg);
static int b_wr(const char *arg);
static int b_rm(const char *arg);
static int b_mkdir(const char *arg);
static int b_dir(const char *arg);
static int b_fork(const char *arg);
static int b_whoami(const char *arg);
static int b_abort(const char *arg);
static int b_stat(const char *arg);
static int b_chmown(const char *arg);
static int b_drop(const char *arg);
static int b_app(const char *arg);
static int b_cp(const char *arg);

static struct builtin builtins[] = {
    {
        "cd",
        "Change working directory",
        b_cd,
    },
    {
        "pwd",
        "Print working directory",
        b_pwd,
    },
    {
        "cat",
        "Print file contents" /* Concatenate files */,
        b_cat
    },
    {
        "wr",
        "",
        b_wr,
    },
    {
        "rm",
        "Remove file/directory",
        b_rm
    },
    {
        "mkdir",
        "Create a directory",
        b_mkdir
    },
    {
        "sleep",
        "Sleep N seconds",
        b_sleep
    },
    {
        "clear",
        "Clear terminal",
        b_clear,
    },
    {
        "dir",
        "Print directory listing",
        b_dir,
    },
    {
        "fork",
        "Test fork",
        b_fork,
    },
    {
        "whoami",
        "Who am I?",
        b_whoami,
    },
    {
        "abort",
        "Suicide",
        b_abort,
    },
    {
        "chmown",
        "chmod + chown",
        b_chmown,
    },
    {
        "stat",
        "Display file/directory information",
        b_stat,
    },
    {
        "drop",
        "Become a peasant",
        b_drop,
    },
    {
        "app",
        "Append a file",
        b_app,
    },
    {
        "cp",
        "Copy a file",
        b_cp,
    },
    {
        "help",
        "Please help me",
        b_help
    },
    { NULL, NULL, NULL }
};

static int b_cd(const char *arg) {
    int res;
    if (!arg) {
        arg = "";
    }

    if ((res = chdir(arg)) < 0) {
        perror(arg);
    }

    return res;
}

static int b_pwd(const char *arg) {
    char buf[512];
    if (!getcwd(buf, sizeof(buf))) {
        perror("getcwd()");
        return -1;
    } else {
        puts(buf);
    }
    return 0;
}

static int rm_node(const char *path) {
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    int res;

    if ((res = stat(path, &st)) != 0) {
        perror(path);
        return res;
    }

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        char child_path[512];

        if (!(dir = opendir(path))) {
            perror(path);
            return -1;
        }

        res = 0;
        while ((ent = readdir(dir))) {
            if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
                snprintf(child_path, sizeof(child_path), "%s/%s", path, ent->d_name);
                if ((res = rm_node(child_path)) != 0) {
                    break;
                }
            }
        }

        closedir(dir);

        if (res == 0) {
            if ((res = rmdir(path)) != 0) {
                perror(path);
            }
        }

        return res;
    } else {
        if ((res = unlink(path)) != 0) {
            perror(path);
        }

        return res;
    }
}

static int b_fork(const char *arg) {
    int pid = fork();

    if (pid < 0) {
        perror("fork()");
        return pid;
    }

    if (pid == 0) {
        printf("Child sleeps\n");
        usleep(10000000);
        printf("Child is done sleeping\n");
        exit(0);
    }

    printf("Child pid: %d\n", pid);

    return 0;
}

static int b_chmown(const char *arg) {
    mode_t accmode = 0;
    uid_t uid = 0;
    gid_t gid = 0;

    char buf[64];
    int res;

    if (!arg) {
        return -1;
    }

    // Check if file exists
    if ((res = access(arg, F_OK)) != 0) {
        perror(arg);
        return res;
    }

    printf("Mode: ");
    if ((res = readline(buf, sizeof(buf))) < 0) {
        return res;
    }
    if ((res = from_oct((int *) &accmode, buf)) != 0) {
        return res;
    }

    printf("UID: ");
    if ((res = readline(buf, sizeof(buf))) < 0) {
        return res;
    }
    uid = atoi(buf);

    printf("GID: ");
    if ((res = readline(buf, sizeof(buf))) < 0) {
        return res;
    }
    gid = atoi(buf);

    if ((res = chown(arg, uid, gid)) != 0) {
        perror(arg);
        return res;
    }

    if ((res = chmod(arg, accmode)) != 0) {
        perror(arg);
        return res;
    }

    return 0;
}

static char *basename(char *path) {
    char *p = strrchr(path, '/');
    if (!p) {
        return path;
    } else {
        return p + 1;
    }
}

static int b_cp(const char *arg) {
    char src_path[256];
    char dst_path[256];
    struct stat st;
    int fd_src, fd_dst;
    int res;

    const char *spc = strchr(arg, ' ');
    if (!spc) {
        printf("Usage: cp <src> <dst>");
        return -1;
    }

    strncpy(src_path, arg, spc - arg);
    strcpy(dst_path, spc + 1);

    // Check that source is a file
    if ((res = stat(src_path, &st)) != 0) {
        perror(src_path);
        return res;
    } else if ((st.st_mode & S_IFMT) != S_IFREG) {
        printf("Not a file: %s\n", src_path);
        return -1;
    }

    // Check if destination is a directory
    if ((res = stat(dst_path, &st)) == 0) {
        if ((st.st_mode & S_IFMT) == S_IFDIR) {
            const char *name = basename(src_path);
            size_t len = strlen(dst_path);
            dst_path[len++] = '/';
            strcpy(&dst_path[len], name);
            // TODO: check that this file is not some kind of a special device
        } else if ((st.st_mode & S_IFMT) != S_IFREG) {
            printf("Invalid destination: %s\n", dst_path);
            return -1;
        }
    }

    if ((fd_src = open(src_path, O_RDONLY, 0)) < 0) {
        perror(src_path);
        return -1;
    }

    // TODO: does cp copy file mode?
    if ((fd_dst = open(dst_path, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0) {
        close(fd_src);
        perror(dst_path);
        return -1;
    }

    char buf[512];
    ssize_t bread;
    ssize_t bwrite;

    while ((bread = read(fd_src, buf, sizeof(buf))) > 0) {
        if ((bwrite = write(fd_dst, buf, bread)) != bread) {
            printf("Write failed\n");
            break;
        }
    }

    close(fd_dst);
    close(fd_src);

    return 0;
}

static int b_app(const char *arg) {
    if (!arg) {
        return -1;
    }

    int opt = O_APPEND | O_WRONLY;

    while (1) {
        while (isspace(*arg)) {
            ++arg;
        }
        if (arg[0] == '-' && isalpha(arg[1])) {
            // Some flag
            switch (arg[1]) {
            case 'c':
                opt |= O_CREAT;
                break;
            case 't':
                opt |= O_TRUNC;
                break;
            default:
                printf("Unknown flag: -%c\n", arg[1]);
                return -1;
            }
            arg += 2;
        } else {
            break;
        }
    }

    int fd = open(arg, opt, 0644);
    if (fd < 0) {
        perror(arg);
        return fd;
    }

    printf("Type \"EOF\" to stop writing\n");

    char buf[512];
    int line_len;

    while (1) {
        if ((line_len = readline(buf, sizeof(buf))) < 0) {
            break;
        }

        if (!strcmp(buf, "EOF")) {
            break;
        }

        if (write(fd, buf, line_len) < 0) {
            printf("Write failed\n");
            break;
        }
        *(char *) &line_len = '\n';
        write(fd, &line_len, 1);
    }

    close(fd);

    return 0;
}

static int b_rm(const char *arg) {
    int res;
    int is_r = 0;
    if (!arg) {
        return -1;
    }

    if (!strncmp(arg, "-r ", 3)) {
        arg += 3;
        is_r = 1;
    }

    if (!is_r) {
        if ((res = unlink(arg)) < 0) {
            perror(arg);
        }
    } else {
        rm_node(arg);
    }

    return res;
}

static int b_mkdir(const char *arg) {
    int res;
    if (!arg) {
        return -1;
    }

    if ((res = mkdir(arg, 0755)) < 0) {
        perror(arg);
    }

    return res;
}

static int b_dir(const char *arg) {
    assert(arg);
    DIR *dir;
    struct dirent *ent;

    if (!(dir = opendir(arg))) {
        perror(arg);
        return -1;
    }

    while ((ent = readdir(dir))) {
        printf("%c %s\n", ent->d_type == DT_DIR ? 'd' : 'f', ent->d_name);
    }

    closedir(dir);

    return 0;
}

static int b_abort(const char *arg) {
    abort();
    return 0;
}

static int b_stat(const char *arg) {
    if (!arg) {
        return -1;
    }

    struct stat st;
    int res;

    if ((res = stat(arg, &st)) != 0) {
        perror(arg);
        return res;
    }

    printf("dev     %u\n", st.st_dev);
    printf("rdev    %u\n", st.st_rdev);

    printf("inode   %u\n", st.st_ino);

    printf("mode    %u\n", st.st_mode);
    printf("nlink   %u\n", st.st_nlink);

    printf("uid     %u\n", st.st_uid);
    printf("gid     %u\n", st.st_gid);

    printf("size    %u\n", st.st_size);
    printf("blocks  %u\n", st.st_blocks);
    printf("blksize %u\n", st.st_blksize);

    printf("atime   %u\n", st.st_atime);
    printf("mtime   %u\n", st.st_mtime);
    printf("ctime   %u\n", st.st_ctime);

    return 0;
}

static int b_wr(const char *arg) {
    int fd;
    char buf[512];
    int line_len;

    if (!arg) {
        return -1;
    }

    if ((fd = open(arg, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0) {
        perror(arg);
        return fd;
    }

    printf("Type \"EOF\" to stop writing\n");

    while (1) {
        if ((line_len = readline(buf, sizeof(buf))) < 0) {
            break;
        }

        if (!strcmp(buf, "EOF")) {
            break;
        }

        if (write(fd, buf, line_len) != line_len) {
            break;
        }
        char c = '\n';
        write(fd, &c, 1);
    }

    close(fd);

    return 0;
}

static int b_drop(const char *arg) {
    int res;

    if ((res = setuid(1000)) != 0) {
        perror("setuid()");
        return res;
    }
    if ((res = setgid(1000)) != 0) {
        perror("setgid()");
        return res;
    }

    return 0;
}

static int b_cat(const char *arg) {
    char buf[512];
    int fd;
    ssize_t bread;

    if (!arg) {
        return -1;
    }

    if ((fd = open(arg, O_RDONLY, 0)) < 0) {
        perror(arg);
        return fd;
    }

    while ((bread = read(fd, buf, sizeof(buf))) > 0) {
        write(STDOUT_FILENO, buf, bread);
    }

    close(fd);

    return 0;
}

static int b_sleep(const char *arg) {
    if (!arg) {
        return -1;
    }
    int seconds = 0;
    while (*arg) {
        seconds *= 10;
        seconds += *arg - '0';
        ++arg;
    }

    struct timespec ts = { seconds, 0 };
    if ((seconds = nanosleep(&ts, NULL))) {
        perror("nanosleep()");
        return seconds;
    }

    return 0;
}

static int b_clear(const char *arg) {
    clear();
    curs_set(1, 1);
    return 0;
}

static int b_whoami(const char *arg) {
    uid_t uid = getuid();
    gid_t gid = getgid();

    printf("%d:%d\n", uid, gid);

    return 0;
}

static int b_help(const char *arg) {
    if (arg) {
        // Describe a specific command
        for (size_t i = 0; builtins[i].run; ++i) {
            if (!strcmp(arg, builtins[i].name)) {
                printf("%s: %s\n", builtins[i].name, builtins[i].desc);
                return 0;
            }
        }

        printf("%s: Unknown command\n", arg);
        return -1;
    } else {
        for (size_t i = 0; builtins[i].run; ++i) {
            printf("%s: %s\n", builtins[i].name, builtins[i].desc);
        }

        return 0;
    }
}

static void prompt(void) {
    char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) {
        cwd[0] = '?';
        cwd[1] = 0;
    }
    printf("\033[36mygg\033[0m %s > ", cwd);
}

static int cmd_subproc_exec(const char *abs_path, const char *cmd, const char *e) {
    // Maximum of 8 arguments 64 chars each (63)
    // Split input argument into pieces by space
    char args[64 * 8] = {0};
    const char *p = e;
    const char *t = NULL;
    size_t argc = 0;
    while (p) {
        t = strchr(p, ' ');
        if (!t) {
            // Last argument
            assert(strlen(p) < 64);
            strcpy(&args[argc++ * 64], p);
            break;
        } else {
            assert(t - p < 64);
            strncpy(&args[argc * 64], p, t - p);
            args[(argc++ * 64) + (t - p)] = 0;
            p = t + 1;
            while (*p == ' ') {
                ++p;
            }
            if (!*p) {
                break;
            }
        }
    }

    // Fill arg pointer array
    const char *argp[10] = { cmd };
    for (size_t i = 0; i < argc; ++i) {
        argp[i + 1] = &args[i * 64];
    }
    argp[argc + 1] = NULL;

    int pid = fork();
    int status;

    switch (pid) {
    case -1:
        perror("fork()");
        return -1;
    case 0:
        if (execve(abs_path, argp, NULL) != 0) {
            perror("execve()");
        }
        exit(-1);
    default:
        if (waitpid(pid, &status) != 0) {
            perror("waitpid()");
        }
        if (status != 0) {
            printf("%d finished with status: %d\n", pid, status);
        }
        return 0;
    }
}

static int cmd_exec(const char *line) {
    char cmd[64];
    const char *e = strchr(line, ' ');

    if (!e) {
        assert(strlen(line) < 64);
        strcpy(cmd, line);
    } else {
        assert(e - line < 64);
        strncpy(cmd, line, e - line);
        cmd[e - line] = 0;
        ++e;
        while (*e == ' ') {
            ++e;
        }
    }

    if (e && !*e) {
        e = NULL;
    }

    for (size_t i = 0; builtins[i].run; ++i) {
        if (!strcmp(cmd, builtins[i].name)) {
            return builtins[i].run(e);
        }
    }

    // If command starts with ./ or /, try to execute it
    if (((cmd[0] == '.' && cmd[1] == '/') || cmd[0] == '/') && access(cmd, X_OK) == 0) {
        return cmd_subproc_exec(cmd, cmd, e);
    }
    // Try to execute binary from /bin
    // if name has no slashes
    if (!strchr(cmd, '/')) {
        char path_buf[512];
        snprintf(path_buf, sizeof(path_buf), "/bin/%s", cmd);
        if (access(path_buf, X_OK) == 0) {
            return cmd_subproc_exec(path_buf, cmd, e);
        }
    }

    printf("%s: Unknown command\n", cmd);
    return -1;
}

int main(int argc, char **argv) {
    if (getpid() != 1) {
        printf("Won't work if PID is not 1\n");
        return -1;
    }

    char linebuf[512];
    char c;
    size_t l = 0;
    int res;

    // Try to mount devfs
    const char *automount_dev = "/dev/sda1";

    if ((res = mount(NULL, "/dev", "devfs", 0, NULL)) != 0) {
        perror("Failed to mount devfs");
    }
    // Mount sda1 if present
    if (access(automount_dev, F_OK) == 0) {
        printf("%s exists, trying to mount at /mnt\n", automount_dev);

        if ((res = mount(automount_dev, "/mnt", "ext2", 0, NULL)) != 0) {
            perror(automount_dev);
        }
    }

    //setuid(1000);
    //setgid(1000);

    prompt();

#if 0
    if ((res = fork()) < 0) {
        perror("fork()");
        return -1;
    } else if (res == 0) {
        if (execve("/time", NULL, NULL) != 0) {
            perror("execve()");
        }
        exit(-1);
    }
#endif

    while (1) {
        if (read(STDIN_FILENO, &c, 1) < 0) {
            printf("Read failed\n");
            break;
        }

        if (c == '\b') {
            if (l) {
                linebuf[--l] = 0;
                printf("\033[D \033[D");
            }
            continue;
        }
        if (c == '\n') {
            write(STDOUT_FILENO, &c, 1);
            linebuf[l] = 0;

            if (!strcmp(linebuf, "exit")) {
                break;
            }

            l = 0;
            if ((res = cmd_exec(linebuf)) != 0) {
                printf("\033[31m= %d\033[0m\n", res);
            }
            prompt();
            continue;
        }

        linebuf[l++] = c;
        write(STDOUT_FILENO, &c, 1);
    }

    return -1;
}
