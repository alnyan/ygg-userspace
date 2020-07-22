#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#define FS_PROC     "/sys/proc"

int main(int argc, char **argv) {
    FILE *fp;
    DIR *dir;
    struct dirent *ent;
    char *p;
    char buf[256];

    if (!(dir = opendir(FS_PROC))) {
        perror(FS_PROC);
        return -1;
    }

    printf("PID    UID:GID     NAME\n");
    while ((ent = readdir(dir))) {
        if (isdigit(ent->d_name[0])) {
            printf("#%-5s ", ent->d_name);

            // Process UID:GID
            snprintf(buf, sizeof(buf), FS_PROC "/%s/ioctx", ent->d_name);
            fp = fopen(buf, "r");
            if (!fp) {
                perror(buf);
                fputs("????:????", stdout);
            } else {
                if (fgets(buf, sizeof(buf), fp)) {
                    if (buf[0] && (p = strchr(buf, '\n'))) {
                        *p = 0;
                    }
                    printf("%-11s ", buf);
                } else {
                    fputs("????:????", stdout);
                }

                fclose(fp);
            }

            // Process name
            snprintf(buf, sizeof(buf), FS_PROC "/%s/name", ent->d_name);
            fp = fopen(buf, "r");
            if (!fp) {
                perror(buf);
                fputs("???\n", stdout);
            } else {
                if (fgets(buf, sizeof(buf), fp)) {
                    fputs(buf, stdout);
                } else {
                    fputs("???\n", stdout);
                }

                fclose(fp);
            }
        }
    }

    closedir(dir);
    return 0;
}
