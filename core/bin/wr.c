#include <stdio.h>

int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "w");
    if (!fp) {
        perror(argv[1]);
        return -1;
    }

    fwrite("123\n", 1, 4, fp);

    fclose(fp);
    return 0;
}
