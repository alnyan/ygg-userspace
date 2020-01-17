#include <dirent.h>
#include <string.h>
#include <stdio.h>

#define RM_RECURSIVE    (1 << 0)

static int rm_node(const char *path, int flags) {
    struct stat st;

    if (stat(path, &st) != 0) {
        perror(path);
        return -1;
    }

    if ((st.st_mode & S_IFMT) == S_IFDIR) {
        if (!(flags & RM_RECURSIVE)) {
            printf("%s: Is a directory\n", path);
            return -1;
        }

        DIR *dir = opendir(path);
        if (!dir) {
            perror(path);
            return -1;
        }

        struct dirent *ent;
        char child_path[256];
        int yes = 1;

        while ((ent = readdir(dir))) {
            if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
                snprintf(child_path, sizeof(child_path), "%s/%s", path, ent->d_name);

                if (rm_node(child_path, flags) != 0) {
                    yes = 0;
                }
            }
        }

        closedir(dir);

        if (!yes) {
            printf("%s: Is a directory\n", path);
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

int main(int argc, char **argv) {
    // 1 - recurse
    int flags = 0;
    int c;

    if (argc <= 1) {
        printf("usage: rm [-r] <path> ...\n");
        return -1;
    }

    while ((c = getopt(argc, argv, "r")) != -1) {
        switch (c) {
        case 'r':
            flags |= RM_RECURSIVE;
            break;
        default:
            printf("usage: rm [-r] <path> ...");
            return -1;
        }
    }

    for (int i = optind; i < argc; ++i) {
        rm_node(argv[i], flags);
    }

    return 0;
}
