#include <sys/netctl.h>
#include <errno.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc == 2) {
        uint32_t inaddr;

        if (netctl(argv[1], NETCTL_GET_INADDR, &inaddr) != 0) {
            if (errno == ENOENT) {
                printf("No address\n");
                return 0;
            }
            perror("netctl()");
            return -1;
        }

        printf("%d.%d.%d.%d\n", inaddr >> 24, (inaddr >> 16) & 0xFF, (inaddr >> 8) & 0xFF, inaddr & 0xFF);
        return 0;
    } else if (argc == 3) {
        uint32_t inaddr_bytes[4];

        if (sscanf(argv[2], "%d.%d.%d.%d", &inaddr_bytes[0], &inaddr_bytes[1], &inaddr_bytes[2], &inaddr_bytes[3]) != 4) {
            printf("Invalid address format\n");
            return -1;
        }

        uint32_t inaddr = (inaddr_bytes[0] & 0xFF) << 24 |
                          (inaddr_bytes[1] & 0xFF) << 16 |
                          (inaddr_bytes[2] & 0xFF) << 8 |
                          (inaddr_bytes[3] & 0xFF);


        if (netctl(argv[1], NETCTL_SET_INADDR, &inaddr) != 0) {
            perror("netctl()");
            return -1;
        }

        return 0;
    } else {
        printf("Usage: netctl <interface> [<address>]\n");
        return -1;
    }
}
