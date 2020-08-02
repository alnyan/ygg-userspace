extern int dyn_add(int, int);
extern int dyn_mul(int, int);

void _start(void *arg) {
    (void) arg;
    int r = dyn_add(1, 2);
    r = dyn_mul(r, 3);
    dyn_add(r, 4);
    while (1);
}
