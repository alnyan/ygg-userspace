#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#define OPT_REVERSE     (1 << 1)
#define OPT_COUNT       (1 << 2)
#define OPT_DIR_SUFFIX  (1 << 3)
#define OPT_NUMERIC     (1 << 4)
#define OPT_INO         (1 << 5)
#define OPT_UID         (1 << 6)
#define OPT_GID         (1 << 7)
#define OPT_HIDDEN      (1 << 8)
#define OPT_DOTS        (1 << 9)
#define OPT_NOLSTAT     (1 << 10)
#define OPT_NOLSTATOP   (1 << 11)
#define OPT_CTIME       (1 << 12)
#define OPT_NOLS        (1 << 13)
#define OPT_NOSORT      (1 << 14)
#define OPT_NONPRINT    (1 << 15)
#define OPT_BLOCKS      (1 << 16)
#define OPT_ATIME       (1 << 17)
#define OPT_DST_SUFFIX  (1 << 18)

#define VALID_STAT      (1 << 0)

enum sort_key {
    SORT_NAME = 0,
    SORT_SIZE,
    SORT_TIME
};

enum format {
    FMT_SHORT = 0,
    FMT_LONG = 1,
    FMT_COMMA = 2
};

struct options {
    enum sort_key sort_key;
    enum format format;
    int flags;
    size_t blk_size;
};

struct entry {
    int valid, type;
    char name[256];
    struct stat stat;
};

#define VEC_INITIAL     16
#define VEC_GROW(n)     ((n) + 16)
struct entry_vector {
    size_t cap, size;
    struct entry *data;
};

static struct entry *entry_vector_new(struct entry_vector *vec) {
    if (vec->cap == vec->size) {
        vec->cap = VEC_GROW(vec->cap);
        vec->data = realloc(vec->data, vec->cap * sizeof(struct entry));
        if (!vec->data) {
            return NULL;
        }
    }

    return &vec->data[vec->size++];
}

static int entry_vector_init(struct entry_vector *vec) {
    vec->cap = VEC_INITIAL;
    vec->size = 0;
    vec->data = malloc(vec->cap * sizeof(struct entry));
    return !vec->data;
}

static int entry_sort_comparator(const void *v0, const void *v1) {
    const char *name0 = ((struct entry *) v0)->name;
    const char *name1 = ((struct entry *) v1)->name;
    if (*name0 == '.') {
        ++name0;
    }
    if (*name1 == '.') {
        ++name1;
    }
    return strcmp(name0, name1);
}

static void entry_print(const struct options *opt, const struct entry *ent, int last) {
    char buf[64];
    struct tm tm;
    char t;

    // TODO: show "total N"

    // For both formats
    if (opt->flags & OPT_INO) {
        if (ent->valid & VALID_STAT) {
            printf("% 6u ", ent->stat.st_ino);
        } else {
            printf("?????? ");
        }
    }

    if (opt->flags & OPT_BLOCKS) {
        // TODO: honor -k
        if (ent->valid & VALID_STAT) {
            printf("% 4u ", ent->stat.st_blocks);
        } else {
            printf("???? ");
        }
    }

    if (opt->format == FMT_LONG) {
        // TODO: assuming this type is set correctly
        switch (ent->type) {
        case DT_DIR:
            t = 'd';
            break;
        case DT_LNK:
            t = 'l';
            break;
        default:
            t = '-';
        }

        if (ent->valid & VALID_STAT) {
            // File mode
            printf("%c%c%c%c%c%c%c%c%c%c ", t,
                   (ent->stat.st_mode & S_IRUSR)?'r':'-',
                   (ent->stat.st_mode & S_IWUSR)?'w':'-',
                   (ent->stat.st_mode & S_IXUSR)?'x':'-',
                   (ent->stat.st_mode & S_IRGRP)?'r':'-',
                   (ent->stat.st_mode & S_IWGRP)?'w':'-',
                   (ent->stat.st_mode & S_IXGRP)?'x':'-',
                   (ent->stat.st_mode & S_IROTH)?'r':'-',
                   (ent->stat.st_mode & S_IWOTH)?'w':'-',
                   (ent->stat.st_mode & S_IXOTH)?'x':'-');

            // Number of links
            printf("% 3u ", ent->stat.st_nlink);

            if (opt->flags & OPT_UID) {
                if (opt->flags & OPT_NUMERIC) {
                    printf("%4u ", ent->stat.st_uid);
                } else {
                    // TODO: get owner name
                    printf("owner  ");
                }
            }
            if (opt->flags & OPT_GID) {
                if (opt->flags & OPT_NUMERIC) {
                    printf("%4u ", ent->stat.st_gid);
                } else {
                    // TODO: get group name
                    printf("group  ");
                }
            }

            printf("% 7u ", ent->stat.st_size);

            time_t time = ent->stat.st_mtime;

            // TODO: "if the file has been modified in the last six months, or ..."
            if (localtime_r(&time, &tm)) {
                strftime(buf, sizeof(buf), "%b %e %H:%M", &tm);
                printf("%s ", buf);
            } else {
                printf("??? ?? ????? ");
            }
        } else {
            printf("<error> ");
        }
    }

    printf("%s", ent->name);

    if (opt->flags & OPT_DIR_SUFFIX) {
        if (ent->type == DT_DIR) {
            fputc('/', stdout);
        }
    }

    if (opt->format == FMT_COMMA && !last) {
        printf(", ");
    } else {
        printf("\n");
    }
}

static int stat_ent(const struct options *opt, const char *path, struct stat *st) {
    return (opt->flags & OPT_NOLSTAT) ? stat(path, st) : lstat(path, st);
}

static int stat_op(const struct options *opt, const char *path, struct stat *st) {
    return (opt->flags & OPT_NOLSTATOP) ? stat(path, st) : lstat(path, st);
}

static void fetch_entry(const struct options *opt, struct entry *ent, const char *path, int is_operand) {
    ent->valid = 0;
    ent->type = DT_REG;

    if ((is_operand ? stat_op : stat_ent)(opt, path, &ent->stat) == 0) {
        ent->valid |= VALID_STAT;
    }

    switch (ent->stat.st_mode & S_IFMT) {
    case S_IFDIR:
        ent->type = DT_DIR;
        break;
    default:
        // TODO: fetch from stat
        ent->type = DT_REG;
        break;
    }
}

static int ls_dir(const struct options *opt, const char *path, int last) {
    if (!(opt->flags & OPT_NOLS)) {
        DIR *dir;
        struct dirent *e;
        char ent_path[512];
        struct entry_vector entries;

        if (entry_vector_init(&entries) != 0) {
            return -1;
        }

        if (!(dir = opendir(path))) {
            perror(path);
            return -1;
        }

        while ((e = readdir(dir))) {
            if (e->d_name[0] == '.' && !(opt->flags & (OPT_HIDDEN | OPT_DOTS))) {
                continue;
            }
            if ((!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) &&
                !(opt->flags & OPT_DOTS)) {
                continue;
            }

            struct entry *ent = entry_vector_new(&entries);
            if (!ent) {
                break;
            }
            strcpy(ent->name, e->d_name);

            snprintf(ent_path, sizeof(ent_path), "%s/%s",
                     strcmp(path, "") ? path : ".", e->d_name);
            fetch_entry(opt, ent, ent_path, 0);
        }

        closedir(dir);

        // Sort entries
        if (!(opt->flags & OPT_NOSORT)) {
            // TODO: comparator kinds
            qsort(entries.data, entries.size, sizeof(struct entry), entry_sort_comparator);
        }

        for (size_t i = 0; i < entries.size; ++i) {
            entry_print(opt, &entries.data[i], i == entries.size - 1);
        }

        free(entries.data);
    } else {
        // Fetch single entry: operand itself
        struct entry ent;
        strcpy(ent.name, path);
        fetch_entry(opt, &ent, path, 1);

        entry_print(opt, &ent, last);
    }

    return 0;
}

void usage(void) {
    fprintf(stderr, "usage: ls [-ikqrs] [-glno] [-A|-a] [-C|-m|-x|-1] \\\n");
    fprintf(stderr, "       [-F|-p] [-H|-L] [-R|-d] [-S|-f|-t] [-c|-u] [file...]\n");
}

int main(int argc, char **argv) {
    int ch;
    static const char *opts = "ikqrsglnoAaCmx1FpHLRdSftcu";
    struct options options = {
        .sort_key = SORT_NAME,
        .format = FMT_SHORT,
        .flags = 0,
        .blk_size = 0,
    };

    while ((ch = getopt(argc, argv, opts)) != -1) {
        switch (ch) {
        case 'A':                                               // DONE
            options.flags |= OPT_HIDDEN;
            break;
        case 'C':                                               // ????
            // Ignored: no column output
            if (options.format == FMT_LONG) {
                options.format = FMT_SHORT;
            }
            break;
        case 'F':                                               // ????
            // Don't follow operand-symlinks
            // Suffices
            options.flags |= OPT_DST_SUFFIX;
            break;
        case 'H':                                               // ----
            options.flags |= OPT_NOLSTATOP;
            break;
        case 'L':                                               // ----
            options.flags |= OPT_NOLSTATOP | OPT_NOLSTAT;
            break;
        case 'R':                                               // ----
            fprintf(stderr, "Not implemented\n");
            return -1;
        case 'S':                                               // ----
            options.sort_key = SORT_SIZE;
            break;
        case 'a':                                               // DONE
            options.flags |= OPT_HIDDEN | OPT_DOTS;
            break;
        case 'c':                                               // ----
            options.flags |= OPT_CTIME;
            break;
        case 'd':                                               // DONE
            // Only show info about operands
            options.flags |= OPT_NOLS;
            break;
        case 'f':                                               // ????
            options.flags |= OPT_NOSORT;
            break;
        case 'g':                                               // DONE
            options.flags &= ~OPT_UID;
            options.flags |= OPT_GID;
            options.format = FMT_LONG;
            break;
        case 'i':                                               // DONE
            options.flags |= OPT_INO;
            break;
        case 'k':                                               // ????
            options.blk_size = 1024;
            break;
        case 'n':                                               // DONE
            options.flags |= OPT_NUMERIC;
            __attribute__((fallthrough));
        case 'l':                                               // DONE
            options.flags |= OPT_GID | OPT_UID;
            options.format = FMT_LONG;
            break;
        case 'm':                                               // DONE
            options.format = FMT_COMMA;
            break;
        case 'o':                                               // DONE
            options.flags &= ~OPT_GID;
            options.flags |= OPT_UID;
            options.format = FMT_LONG;
            break;
        case 'p':                                               // ----
            options.flags |= OPT_DIR_SUFFIX;
            break;
        case 'q':                                               // ----
            options.flags |= OPT_NONPRINT;
            break;
        case 'r':                                               // ----
            options.flags |= OPT_REVERSE;
            break;
        case 's':                                               // DONE
            options.flags |= OPT_BLOCKS;
            break;
        case 't':                                               // ----
            options.sort_key = SORT_TIME;
            break;
        case 'u':                                               // ----
            options.flags |= OPT_ATIME;
            break;
        case 'x':                                               // ????
            // Multi-column not implemented
            if (options.format == FMT_LONG) {
                options.format = FMT_SHORT;
            }
            break;
        case '1':                                               // DONE
            // Multi-column not implemented
            break;
        case '?':
            usage();
            return -1;
        }
    }

    if (optind == argc) {
        ls_dir(&options, ".", 1);
    } else {
        for (int i = optind; i < argc; ++i) {
            if (!(options.flags & OPT_NOLS) && (optind != argc - 1)) {
                if (i != optind) {
                    printf("\n");
                }
                printf("%s:\n", argv[i]);
            }
            ls_dir(&options, argv[i], i == argc - 1);
        }
    }

    return 0;
}
