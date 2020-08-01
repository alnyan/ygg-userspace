

static inline long __syscall3(long n, long a1, long a2, long a3) {
    unsigned long ret;
    asm volatile ("syscall"
                 :"=a"(ret)
                 :"a"(n),"D"(a1),"S"(a2),"d"(a3)
                 :"rcx","r11","memory");
    return ret;
}

static inline long __syscall1(long n, long a1) {
    unsigned long ret;
    asm volatile ("syscall"
                 :"=a"(ret)
                 :"a"(n),"D"(a1)
                 :"rcx","r11","memory");
    return ret;
}

char data[128] = {0};

void _start(void *arg) {
    (void) arg;
    const char *src = "abcde!\n";
    for (long i = 0; i < 128; ++i) {
        data[i] = src[i];
        if (!src[i]) {
            break;
        }
    }
    __syscall3(1, 1, (long) data, 7);
    __syscall1(60, 0);
    while (1);
}
