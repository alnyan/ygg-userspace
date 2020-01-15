#include <stdio.h>

static void usage(void) {
    printf("Usage: mount [-t type] device target\n");
}

int main(int argc, char **argv) {
    int res;
    struct stat st;
    const char *type = NULL;
    const char *src = NULL;
    const char *dst = NULL;
    uid_t uid;

    // Crude attempt at getopt()
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 't':
                if (i == argc - 1) {
                    usage();
                    return -1;
                }
                type = argv[i + 1];
                ++i;
                break;
            default:
                return -1;
            }

            continue;
        }

        if (!src) {
            src = argv[i];
        } else if (!dst) {
            dst = argv[i];
        } else {
            usage();
            return -1;
        }
    }

    if (!dst) {
        dst = src;
        src = NULL;
    }
    if (!dst) {
        usage();
        return -1;
    }

    // Check that UID is 0
    if ((uid = getuid()) != 0) {
        printf("mount should be run as root\n");
        return -1;
    }

    if (src) {
        // Check that "src" is a block device
        if ((res = stat(src, &st)) != 0) {
            perror(src);
            return -1;
        }

        if ((st.st_mode & S_IFMT) != S_IFBLK) {
            printf("%s: Not a block device\n", src);
            return -1;
        }
    }

    // Check that "dst" is a directory
    if ((res = stat(dst, &st)) != 0) {
        perror(dst);
        return -1;
    }

    if ((st.st_mode & S_IFMT) != S_IFDIR) {
        printf("%s: Not a directory\n", dst);
        return -1;
    }

    // Do the magic stuff
    if ((res = mount(src, dst, type, 0, NULL)) != 0) {
        perror("mount()");
        return -1;
    }

    return 0;
}
