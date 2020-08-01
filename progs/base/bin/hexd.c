#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#define LINE_LENGTH     16
#define OPT_CHARS       (1 << 0)

static void line_print(int flags, size_t off, const char *line, size_t len) {
    uint8_t byte;
    printf("%08zx: ", off);
    for (size_t i = 0; i < LINE_LENGTH; ++i) {
        byte = line[i];
        if (i < len) {
            printf("%02hhx", byte);
        } else {
            printf("  ");
        }
        if (i % 2) {
            printf(" ");
        }
    }

    if (flags & OPT_CHARS) {
        printf("| ");
        for (size_t i = 0; i < len; ++i) {
            byte = line[i];
            if (isprint(byte)) {
                printf("%c", byte);
            } else {
                printf(".");
            }
        }
    }

    printf("\n");
}

static int do_file(int flags, const char *pathname) {
    char buf[LINE_LENGTH];
    size_t bread, off;
    FILE *fp;

    if (!strcmp(pathname, "-")) {
        fp = stdin;
    } else {
        fp = fopen(pathname, "rb");
    }
    if (!fp) {
        perror(pathname);
        return -1;
    }

    off = 0;
    while ((bread = fread(buf, 1, sizeof(buf), fp)) > 0) {
        line_print(flags, off, buf, bread);
        off += bread;
    }

    if (fp != stdin) {
        fclose(fp);
    }

    return 0;
}

static void usage(char **argv) {
    fprintf(stderr, "usage: %s [-q] [FILE...]", argv[0]);
}

int main(int argc, char **argv) {
    static const char *opts = "q";
    int flags = OPT_CHARS;
    int ch;

    while ((ch = getopt(argc, argv, opts)) != -1) {
        switch (ch) {
        case 'q':
            flags &= ~OPT_CHARS;
            break;
        case '?':
            usage(argv);
            return -1;
        }
    }

    if (optind == argc) {
        do_file(flags, "-");
    } else {
        for (int i = optind; i < argc; ++i) {
            do_file(flags, argv[i]);
        }
    }

    return 0;
}
