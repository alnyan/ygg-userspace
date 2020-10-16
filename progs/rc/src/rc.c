#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>

#define DIR_DEFAULT         "/etc/rc.d"

static int rc_script_exec(const char *path, const char *arg) {
    int pid = fork();

    if (pid < 0) {
        warn("fork()");
        return -1;
    }

    if (pid == 0) {
        const char *const argv[] = {
            path, arg, NULL
        };

        exit(execve(argv[0], (char *const *) argv, environ));
    } else {
        int res, t;

        t = waitpid(pid, &res, 0);
        if (t < 0) {
            warn("waitpid()");
            return -1;
        }

        return res;
    }
}

static int rc_load_dir(const char *path, const char *action) {
    DIR *dir = opendir(path);
    if (!dir) {
        warn("opendir(%s)", path);
        return -1;
    }
    struct dirent *ent;
    char script[256];

    // TODO: readdir doesn't guarantee ordering
    while ((ent = readdir(dir))) {
        if (ent->d_name[0] == '.') {
            continue;
        }

        snprintf(script, sizeof(script), "%s/%s", path, ent->d_name);

        rc_script_exec(script, "start");
    }

    closedir(dir);

    return 0;
}

static void usage(const char *name) {
    fprintf(stderr, "usage: %s default|shutdown\n", name ? name : "/sbin/rc");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "default")) {
        return rc_load_dir(DIR_DEFAULT, "start");
    } else {
        usage(argv[0]);
        return -1;
    }
}
