#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#define LS_COLOR        (1 << 0)
#define LS_DETAIL       (1 << 1)
#define LS_TIME         (1 << 2)

#define COLOR_DIR       "\033[36m"
#define COLOR_DEV       "\033[33m"
#define COLOR_UNKNOWN   "\033[43m"

#define COLOR_RESET     "\033[0m"

#define VALID_STAT      (1 << 0)

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

// TODO: reimplement qsort(3) in libc
//static int entry_sort_comparator(const void *v0, const void *v1) {
//    const char *name0 = ((struct entry *) v0)->name;
//    const char *name1 = ((struct entry *) v1)->name;
//    return strcmp(name0, name1);
//}

static void entry_print(const struct entry *ent, int flags) {
    char t;

    if (flags & LS_DETAIL) {
        if (!(ent->valid & VALID_STAT)) {
            printf("??????????    ?    ?        ? ");
        } else {
            switch (ent->stat.st_mode & S_IFMT) {
            case S_IFDIR:
                t = 'd';
                break;
            case S_IFREG:
                t = '-';
                break;
            case S_IFBLK:
                t = 'b';
                break;
            case S_IFCHR:
                t = 'c';
                break;
            case S_IFLNK:
                t = 'l';
                break;
            case S_IFIFO:
                t = 'p';
                break;
            default:
                t = '?';
                break;
            }

            printf("%c%c%c%c%c%c%c%c%c%c ",
                t,
                (ent->stat.st_mode & S_IRUSR) ? 'r' : '-',
                (ent->stat.st_mode & S_IWUSR) ? 'w' : '-',
                (ent->stat.st_mode & S_ISUID ? 's' : (ent->stat.st_mode & S_IXUSR) ? 'x' : '-'),
                (ent->stat.st_mode & S_IRGRP) ? 'r' : '-',
                (ent->stat.st_mode & S_IWGRP) ? 'w' : '-',
                (ent->stat.st_mode & S_IXGRP) ? 'x' : '-',
                (ent->stat.st_mode & S_IROTH) ? 'r' : '-',
                (ent->stat.st_mode & S_IWOTH) ? 'w' : '-',
                (ent->stat.st_mode & S_IXOTH) ? 'x' : '-');

            printf("%4u %4u %8u ",
                ent->stat.st_gid,
                ent->stat.st_uid,
                ent->stat.st_size);
        }
    }

    if (flags & LS_TIME) {
        if (!(ent->valid & VALID_STAT)) {
            printf("??? ?? ???? ????? ");
        } else {
            static const char *const mon_names[12] = {
                "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
            };
            time_t t = ent->stat.st_ctime > ent->stat.st_mtime ?
                ent->stat.st_ctime : ent->stat.st_mtime;
            struct tm tm;
            if (gmtime_r(&t, &tm) == NULL) {
                printf("??? ?? ???? ????? ");
            } else {
                printf("%s %2u %04u %02u:%02u ",
                    mon_names[(tm.tm_mon - 1) % 12],
                    tm.tm_mday,
                    tm.tm_year,
                    tm.tm_hour,
                    tm.tm_min);
            }
        }
    }

    if (flags & LS_COLOR) {
        // TODO: take type from lstat() if available
        switch (ent->type) {
        case DT_REG:
            break;
        case DT_BLK:
        case DT_CHR:
            fputs(COLOR_DEV, stdout);
            break;
        case DT_DIR:
            fputs(COLOR_DIR, stdout);
            break;
        default:
            fputs(COLOR_UNKNOWN, stdout);
            break;
        }
    }
    fputs(ent->name, stdout);
    if (flags & LS_COLOR) {
        fputs(COLOR_RESET, stdout);
    }
    fputc('\n', stdout);
}

static int ls_dir(const char *path, int flags) {
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
        if (e->d_name[0] != '.') {
            struct entry *ent = entry_vector_new(&entries);
            if (!ent) {
                break;
            }
            ent->valid = 0;
            ent->type = e->d_type;
            strcpy(ent->name, e->d_name);

            if (flags & (LS_DETAIL | LS_TIME)) {
                snprintf(ent_path, sizeof(ent_path), "%s/%s",
                    strcmp(path, "") ? path : ".", e->d_name);
                if (lstat(ent_path, &ent->stat) == 0) {
                    ent->valid |= VALID_STAT;
                }
            }
        }
    }

    closedir(dir);

    // Sort entries
    //qsort(entries.data, entries.size, sizeof(struct entry), entry_sort_comparator);

    for (size_t i = 0; i < entries.size; ++i) {
        entry_print(&entries.data[i], flags);
    }

    free(entries.data);

    return 0;
}

int main(int argc, char **argv) {
    // Basic getopt-like features
    int first = -1;
    int flags = 0;

    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] != '-') {
            first = i;
            break;
        } else {
            if (!strcmp(argv[i], "-c")) {
                flags |= LS_COLOR;
            } else if (!strcmp(argv[i], "-d")) {
                flags |= LS_DETAIL;
            } else if (!strcmp(argv[i], "-t")) {
                flags |= LS_TIME;
            } else {
                printf("Unknown option: %s\n", argv[i]);
            }
        }
    }

    if (first == -1) {
        ls_dir("", flags);
    } else {
        for (int i = first; i < argc; ++i) {
            if (first != argc - 1) {
                printf("%s:\n", argv[i]);
            }
            ls_dir(argv[i], flags);
        }
    }

    return 0;
}
