#include <stdio.h>

extern int dyn_add(int x, int y);

int main() {
    printf("2 + 2 is %d\n", dyn_add(2, 2));
    return 0;
}
