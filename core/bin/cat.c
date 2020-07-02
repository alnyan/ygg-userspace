#include <string.h>
#include <stdio.h>

#define CAT_NUMBER      (1 << 0)

static int do_cat(const char *filename, int flags) {
    FILE *fp;
    char buf[1024];
    //size_t bread;
    int line = 0;

    if (filename) {
        fp = fopen(filename, "r");

        if (!fp) {
            perror(filename);
            return -1;
        }
    } else {
        fp = stdin;
    }

    do {
        if (fgets(buf, sizeof(buf), fp) == NULL) {
            break;
        }
        //bread = strlen(buf);

        if (flags & CAT_NUMBER) {
            printf("% 4d ", ++line);
        }

        fputs(buf, stdout);
    } while (1);

    if (filename) {
        fclose(fp);
    }
    return 0;
}

int main(int argc, char **argv) {
    int flags = 0;
    int any_printed = 0;
    int res = 0;

    // TODO: handle '-' argument
    // Parse options
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'n':
                flags |= CAT_NUMBER;
                break;
            default:
                fprintf(stderr, "unknown option: '-%c'\n", argv[i][1]);
                return -1;
            }
        }
    }

    // Print any files from args
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            any_printed = 1;
            if (do_cat(argv[i], flags) != 0) {
                res = -1;
            }
        }
    }

    // If no argument specifies a file, print from stdin
    if (!any_printed) {
        res = do_cat(NULL, flags);
    }

    return res;
}
