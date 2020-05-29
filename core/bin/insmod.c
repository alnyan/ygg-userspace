#include <ygg/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

static __attribute__((optimize("-O0"))) int ygg_module_load(const char *file, const char *args) {
    register uintptr_t rdi asm("rdi") = (uintptr_t) file;
    register uintptr_t rsi asm("rsi") = (uintptr_t) args;
    register uintptr_t rax asm("rax") = SYSCALL_NRX_MODULE_LOAD;
    asm volatile ("syscall":::"rax","rdi","rsi");
    (void) rdi;
    (void) rsi;
    return (int) rax;
}

static __attribute__((optimize("-O0"))) int ygg_module_unload(const char *name) {
    register uintptr_t rdi asm("rdi") = (uintptr_t) name;
    register uintptr_t rax asm("rax") = SYSCALL_NRX_MODULE_UNLOAD;
    asm volatile ("syscall":::"rax","rdi");
    (void) rdi;
    return (int) rax;
}

static const char *usage = "Usage: insmod <module path>\n"
                           "  OR   insmod -r <module name>\n";

int main(int argc, char **argv) {
    int res;

    if (argc > 3 || argc < 2) {
        fprintf(stderr, usage);
        return -1;
    }

    if (getuid() != 0) {
        fprintf(stderr, "Only root can do that\n");
        return -1;
    }

    if (argc == 2) {
        if (access(argv[1], F_OK) != 0) {
            perror(argv[1]);
            return -1;
        }

        if ((res = ygg_module_load(argv[1], NULL)) < 0) {
            fprintf(stderr, "%s: %s\n", argv[1], strerror(-res));
            return -1;
        }
    } else {
        if (strcmp(argv[1], "-r")) {
            fprintf(stderr, usage);
            return -1;
        }

        if ((res = ygg_module_unload(argv[2])) < 0) {
            fprintf(stderr, "%s: %s\n", argv[2], strerror(-res));
            return -1;
        }
    }

    return 0;
}
