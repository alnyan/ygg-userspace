#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include "elf.h"

extern char _ld_start, _ld_end;

struct object {
    //FILE *fp;
    int fd;
    Elf64_Ehdr ehdr;
};

static int elf_read(struct object *obj, void *dst, size_t count, off_t pos) {
    ssize_t bread;
    if (lseek(obj->fd, pos, SEEK_SET) != pos) {
        return -1;
    }
    if ((bread = read(obj->fd, dst, count)) < 0) {
        return -1;
    }
    if ((size_t) bread != count) {
        errno = ENOEXEC;
        return -1;
    }
    return 0;
}

static struct object *object_open(const char *pathname) {
    struct object *obj = calloc(1, sizeof(struct object));

    obj->fd = open(pathname, O_RDONLY, 0);
    if (obj->fd < 0) {
        return NULL;
    }

    if (elf_read(obj, &obj->ehdr, sizeof(Elf64_Ehdr), 0) != 0) {
        close(obj->fd);
        free(obj);
        return NULL;
    }

    return obj;
}

static void object_close(struct object *obj) {
    close(obj->fd);
    free(obj);
}

static int object_load(struct object *obj) {
    Elf64_Phdr phdr;

    printf("%u program headers\n", obj->ehdr.e_phnum);
    for (size_t i = 0; i < obj->ehdr.e_phnum; ++i) {
        off_t phoff = obj->ehdr.e_phoff + i * obj->ehdr.e_phentsize;

        if (elf_read(obj, &phdr, obj->ehdr.e_phentsize, phoff) != 0) {
            return -1;
        }

        switch (phdr.p_type) {
        case PT_LOAD:
            {
                uintptr_t base = phdr.p_vaddr & ~0xFFF;
                size_t size = (((phdr.p_vaddr + phdr.p_memsz + 0xFFF) & ~0xFFF) - base) / 0x1000;

                if (base < (uintptr_t) &_ld_end) {
                    fprintf(stderr, "Segment will overwrite the loader code\n");
                    errno = ENOEXEC;
                    return -1;
                }

                void *addr = mmap((void *) base,
                                  size * 0x1000,
                                  PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                                  -1, 0);
                if (addr == MAP_FAILED) {
                    perror("mmap()");
                    return -1;
                }

                // Load filesz bytes
                if (elf_read(obj, (void *) phdr.p_vaddr, phdr.p_filesz, phdr.p_offset) != 0) {
                    return -1;
                }

                // Zero (memsz - filesz) bytes
                if (phdr.p_memsz > phdr.p_filesz) {
                    memset((void *) phdr.p_vaddr + phdr.p_filesz, 0, phdr.p_memsz - phdr.p_filesz);
                }
            }
            break;
        case PT_DYNAMIC:
            fprintf(stderr, "TODO: handle dynamic binaries\n");
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv) {
    const char *prog;
    struct object *program;

    printf("_ld_start = %p, _ld_end = %p\n", &_ld_start, &_ld_end);

    if (argc < 2) {
        fprintf(stderr, "usage: %s PROGRAM [ARGS...]\n", argv[0]);
        return -1;
    }

    prog = argv[1];
    program = object_open(prog);

    if (!program) {
        fprintf(stderr, "Failed to load program: %s\n", prog);
        return -1;
    }

    if (object_load(program) != 0) {
        fprintf(stderr, "Failed to load object\n");
    }

    void (*entry) (void *p) = (void (*) (void *)) program->ehdr.e_entry;

    object_close(program);

    // TODO: form arg pointer like kernel does
    entry(NULL);

    return 0;
}
