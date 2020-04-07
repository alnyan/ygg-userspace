#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
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

static int ls_dir(const char *path, int flags) {
    DIR *dir;
    struct dirent *ent;
    struct stat ent_stat;
    int stat_res;
    char ent_path[512];
    char t;

    if (!(dir = opendir(path))) {
        perror(path);
        return -1;
    }

    while ((ent = readdir(dir))) {
        if (ent->d_name[0] != '.') {
            if (flags & (LS_DETAIL | LS_TIME)) {
                snprintf(ent_path, sizeof(ent_path), "%s/%s",
                    strcmp(path, "") ? path : ".", ent->d_name);
                stat_res = stat(ent_path, &ent_stat);
            }

            if (flags & LS_DETAIL) {
                if (stat_res != 0) {
                    printf("??????????    ?    ?        ? ");
                } else {
                    switch (ent_stat.st_mode & S_IFMT) {
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
                    default:
                        t = '?';
                        break;
                    }

                    printf("%c%c%c%c%c%c%c%c%c%c ",
                        t,
                        (ent_stat.st_mode & S_IRUSR) ? 'r' : '-',
                        (ent_stat.st_mode & S_IWUSR) ? 'w' : '-',
                        (ent_stat.st_mode & S_ISUID ? 's' : (ent_stat.st_mode & S_IXUSR) ? 'x' : '-'),
                        (ent_stat.st_mode & S_IRGRP) ? 'r' : '-',
                        (ent_stat.st_mode & S_IWGRP) ? 'w' : '-',
                        (ent_stat.st_mode & S_IXGRP) ? 'x' : '-',
                        (ent_stat.st_mode & S_IROTH) ? 'r' : '-',
                        (ent_stat.st_mode & S_IWOTH) ? 'w' : '-',
                        (ent_stat.st_mode & S_IXOTH) ? 'x' : '-');

                    printf("%4u %4u %8u ",
                        ent_stat.st_gid,
                        ent_stat.st_uid,
                        ent_stat.st_size);
                }
            }

            if (flags & LS_TIME) {
                if (stat_res != 0) {
                    printf("??? ?? ???? ????? ");
                } else {
                    static const char *const mon_names[12] = {
                        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                    };
                    time_t t = ent_stat.st_ctime > ent_stat.st_mtime ?
                        ent_stat.st_ctime : ent_stat.st_mtime;
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
                switch (ent->d_type) {
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
            fputs(ent->d_name, stdout);
            if (flags & LS_COLOR) {
                fputs(COLOR_RESET, stdout);
            }
            fputc('\n', stdout);
        }
    }

    closedir(dir);

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
