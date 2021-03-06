#include <sys/reboot.h>
#include <ygg/acpi.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int daemon(void) {
    //fclose(stdout);
    //fclose(stderr);
    //fclose(stdin);

    FILE *fp = fopen("/dev/acpi", "rb");
    if (!fp) {
        perror("/dev/acpi");
        return -1;
    }
    int ch;

    // TODO: fix fopen for tty
    int err = open("/dev/tty", O_WRONLY, 0);
    if (err < 0) {
        perror("failed to open err device\n");
    }

    while ((ch = fgetc(fp)) > 0) {
        switch (ch) {
        case ACEV_POWER_BUTTON:
            if (err >= 0) {
                dprintf(err, "Power button pressed, shutdown in 1s\n");
            }
            sleep(1);
            reboot(YGG_REBOOT_MAGIC1, YGG_REBOOT_MAGIC2, YGG_REBOOT_POWER_OFF, NULL);
            while (1);
        default:
            break;
        }
    }

    fclose(fp);
    return 0;
}

int main(int argc, char **argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork()");
        return -1;
    }

    if (pid == 0) {
        return daemon();
    }

    return 0;
}
