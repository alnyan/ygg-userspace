#include <sys/mount.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <err.h>

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s [-t TYPE] [-o OPTIONS] [DEVICE] TARGET\n", prog);
}

static int do_mount(const char *src, const char *tgt, const char *type, uint32_t opts) {
    struct stat st;
    int res;

    // Check that (if specified) source is a block device
    if (src) {
        // Check that "src" is a block device
        if ((res = stat(src, &st)) != 0) {
            return -1;
        }

        if ((st.st_mode & S_IFMT) != S_IFBLK) {
            errno = ENOTBLK;
            return -1;
        }
    }

    // Check that target is a directory
    if ((res = stat(tgt, &st)) != 0) {
        return -1;
    }
    if ((st.st_mode & S_IFMT) != S_IFDIR) {
        errno = ENOTDIR;
        return -1;
    }

    // Do the magic stuff
    return mount(src, tgt, type, opts, NULL);
}

static int parse_flags(const char *prog, char *str, uint32_t *flags) {
    char *p, *e, *q;

    p = str;
    while (p) {
        e = strchr(p, ',');
        q = strchr(p, '=');

        if (e && q > e) {
            q = 0;
        }
        if (e) {
            *e = 0;
        }
        if (q) {
            *q++ = 0;
        }

        if (!strcmp(p, "ro")) {
            *flags |= MS_RDONLY;
        } else if (!strcmp(p, "sync")) {
            *flags |= MS_SYNCHRONOUS;
        } else {
            fprintf(stderr, "%s: unknown option: %s\n", prog, p);
            return -1;
        }

        if (e) {
            p = e + 1;
        } else {
            break;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    static const char *opts = "t:o:";
    const char *dev = NULL;
    const char *target = NULL;
    const char *type = NULL;
    uint32_t flags = 0;
    int optc;

    if (getuid() != 0) {
        fprintf(stderr, "%s: must be run by root\n", argv[0]);
        exit(-1);
    }

    while ((optc = getopt(argc, argv, opts)) != -1) {
        switch (optc) {
        case 't':
            type = optarg;
            break;
        case 'o':
            if (parse_flags(argv[0], optarg, &flags) != 0) {
                usage(argv[0]);
                exit(-1);
            }
            break;
        default:
        case '?':
            usage(argv[0]);
            exit(-1);
        }
    }

    if (optind == argc - 1) {
        target = argv[optind];
    } else if (optind == argc - 2) {
        dev = argv[optind];
        target = argv[optind + 1];
    } else {
        usage(argv[0]);
        exit(-1);
    }

    if (do_mount(dev, target, type, flags) != 0) {
        err(-1, "mount");
    }

    return 0;
}
