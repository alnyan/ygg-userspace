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

    printf("PID    UID:GID     NAME        PARENT\n");
    while ((ent = readdir(dir))) {
        if (isdigit(ent->d_name[0]) || ent->d_name[0] == '-') {
            printf("#%-5s ", ent->d_name);

            // Process UID:GID
            snprintf(buf, sizeof(buf), FS_PROC "/%s/ioctx", ent->d_name);
            fp = fopen(buf, "r");
            if (!fp || !fgets(buf, sizeof(buf), fp)) {
                if (ent->d_name[0] == '-') {
                    printf("%-11s ", "0:0");
                } else {
                    printf("%-11s", "????:????");
                }
            } else {
                if (buf[0] && (p = strchr(buf, '\n'))) {
                    *p = 0;
                }
                printf("%-11s ", buf);
            }
            if (fp) {
                fclose(fp);
                fp = NULL;
            }

            // Process name
            snprintf(buf, sizeof(buf), FS_PROC "/%s/name", ent->d_name);
            fp = fopen(buf, "r");
            if (!fp || !fgets(buf, sizeof(buf), fp)) {
                printf("%-12s", "???");
            } else {
                if (buf[0] && (p = strchr(buf, '\n'))) {
                    *p = 0;
                }
                if (ent->d_name[0] == '-') {
                    printf("[%-9s] ", buf);
                } else {
                    printf("%-12s", buf);
                }
            }
            if (fp) {
                fclose(fp);
                fp = NULL;
            }

            // Process parent
            snprintf(buf, sizeof(buf), FS_PROC "/%s/parent", ent->d_name);
            fp = fopen(buf, "r");
            if (!fp || !fgets(buf, sizeof(buf), fp)) {
                fputs("???\n", stdout);
            } else {
                fputs(buf, stdout);
            }
            if (fp) {
                fclose(fp);
                fp = NULL;
            }
        }
    }

    closedir(dir);
    return 0;
}
