#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define OPT_RECURSE     (1 << 0)
#define OPT_ASK_ALL     (3 << 1)
#define OPT_ASK_MORE    (2 << 1)
#define OPT_ASK_SOME    (1 << 1)

static int ask(const char *path, const struct stat *st) {
    static char buf[512];

    fprintf(stderr, "remove");
    // Double call, duh
    if (access(path, W_OK) != 0) {
        fprintf(stderr, " write-protected");
    }
    switch (st->st_mode & S_IFMT) {
    case S_IFDIR:
        fprintf(stderr, " directory");
        break;
    case S_IFREG:
        if (!st->st_size) {
            fprintf(stderr, " empty");
        }
        fprintf(stderr, " regular file");
        break;
    case S_IFLNK:
        fprintf(stderr, " symbolic link");
        break;
    default:
        // Any other file
        fprintf(stderr, " file");
        break;
    }

    fprintf(stderr, " '%s'? ", path);

    if (!fgets(buf, sizeof(buf), stdin)) {
        return -1;
    }
    if (strcmp(buf, "y\n") && strcmp(buf, "yes\n")) {
        return -1;
    }
    return 0;
}

static int rment(int flags, const char *path) {
    struct stat st;

    if (lstat(path, &st) != 0) {
        perror(path);
        return -1;
    }

    if (flags & OPT_ASK_SOME) {
        if ((flags & OPT_ASK_MORE) || (access(path, W_OK) != 0)) {
            if (ask(path, &st) != 0) {
                return -1;
            }
        }
    }

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        if (!(flags & OPT_RECURSE)) {
            fprintf(stderr, "%s: %s\n", path, strerror(EISDIR));
            return -1;
        }

        DIR *dir;
        struct dirent *ent;
        char ent_path[1024];
        int err = 0;

        if (!(dir = opendir(path))) {
            perror(path);
            return -1;
        }

        while ((ent = readdir(dir))) {
            if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
                snprintf(ent_path, sizeof(ent_path), "%s/%s", path, ent->d_name);

                if (rment(flags, ent_path) != 0) {
                    ++err;
                }
            }
        }

        closedir(dir);

        if (err) {
            return -1;
        }

        if (rmdir(path) != 0) {
            perror(path);
            return -1;
        }

        return 0;
    } else {
        if (unlink(path) != 0) {
            perror(path);
            return -1;
        }

        return 0;
    }
}

static void usage(char **argv) {
    fprintf(stderr, "usage: %s [-fiRr] file...\n", argv[0]);
}

int main(int argc, char **argv) {
    static const char *opts = "fiRr";
    int flags = OPT_ASK_SOME;
    int ch;

    while ((ch = getopt(argc, argv, opts)) > 0) {
        switch (ch) {
        case 'f':
            flags &= ~OPT_ASK_ALL;
            break;
        case 'i':
            flags |= OPT_ASK_ALL;
            break;
        case 'R':
        case 'r':
            flags |= OPT_RECURSE;
            break;
        case '?':
            usage(argv);
            return -1;
        }
    }

    if (optind == argc) {
        usage(argv);
        return -1;
    }

    for (int i = optind; i < argc; ++i) {
        rment(flags, argv[i]);
    }

    return 0;
}
