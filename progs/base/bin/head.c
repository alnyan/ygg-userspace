#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define PRINT_NAMES     (1 << 0)
#define PRINT_FIRST     (1 << 1)

static void headn(FILE *fp, size_t lim) {
    char *lineptr = NULL;
    size_t len = 0;
    // Line-based
    while (getline(&lineptr, &len, fp) >= 0) {
        fputs(lineptr, stdout);
        // TODO: fix getline
        fputc('\n', stdout);
        if (!--lim) {
            break;
        }
    }
}

static void headc(FILE *fp, size_t lim) {
    int ch, last = '\n';

    while ((ch = fgetc(fp)) != EOF) {
        fputc(ch, stdout);
        last = ch;
        if (!--lim) {
            break;
        }
    }
    if (last != '\n') {
        fputc('\n', stdout);
    }
}

static int head(const char *filename, ssize_t n, ssize_t c, int *flags) {
    FILE *fp;

    if ((*flags & PRINT_NAMES)) {
        if ((*flags & PRINT_FIRST)) {
            *flags &= ~PRINT_FIRST;
        } else {
            fputc('\n', stdout);
        }
        printf("==> %s <==\n", filename);
    }

    if (!strcmp(filename, "-")) {
        fp = stdin;
    } else {
        fp = fopen(filename, "r");
    }
    if (!fp) {
        perror(filename);
        return -1;
    }

    if (n) {
        headn(fp, n);
    } else {
        headc(fp, c);
    }

    if (fp != stdin) {
        fclose(fp);
    }

    return 0;
}

int main(int argc, char **argv) {
    ssize_t n_bytes = 0, n_lines = 0;
    int flags;
    int i = 1;
    int start = 1;

    while (i < argc) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'c':
                if (i == argc - 1) {
                    fprintf(stderr, "%s: -c option requires an argument\n", argv[0]);
                    return EXIT_FAILURE;
                }
                n_bytes = atoi(argv[++i]);
                start = i + 1;
                break;
            case 'n':
                if (i == argc - 1) {
                    fprintf(stderr, "%s: -n option requires an argument\n", argv[0]);
                    return EXIT_FAILURE;
                }
                n_lines = atoi(argv[++i]);
                start = i + 1;
                break;
            }
        }

        ++i;
    }

    if (!n_bytes && !n_lines) {
        return 0;
    }
    if (!!n_bytes == !!n_lines) {
        fprintf(stderr, "%s: cannot use both -c and -n options\n", argv[0]);
        return 0;
    }

    flags = (start < argc - 1 ? PRINT_NAMES : 0) | PRINT_FIRST;
    printf("%d, %d\n", start, argc);
    if (start == argc) {
        head("-", n_lines, n_bytes, &flags);
    } else {
        for (i = start; i < argc; ++i) {
            if (argv[i][0] != '-' || !strcmp(argv[i], "-")) {
                head(argv[i], n_lines, n_bytes, &flags);
            }
        }
    }

    return 0;
}
