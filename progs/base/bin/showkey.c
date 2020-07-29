#include <termios.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv) {
    struct termios t0, t1;
    int ch;

    setvbuf(stdin, NULL, _IONBF, 0);

    if (tcgetattr(STDIN_FILENO, &t0) != 0) {
        perror("tcgetattr()");
        return -1;
    }

    t1 = t0;
    t1.c_lflag &= ~(ICANON | ISIG);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &t1) != 0) {
        perror("tcsetattr()");
        return -1;
    }

    while ((ch = fgetc(stdin)) != EOF) {
        if (ch == 0x04) {
            break;
        }
        printf("  0x%02hhx\n", ch);
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &t0);

    return 0;
}
