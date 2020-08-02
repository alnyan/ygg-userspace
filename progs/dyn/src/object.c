#include <sys/debug.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "object.h"
#include "elf.h"

extern void *object_resolver_entry();

struct object {
    FILE *fp;
    char path[256];

    Elf64_Ehdr    ehdr;
    Elf64_Dyn    *dynamic;
    Elf64_Rela   *jmprela;
    Elf64_Sym    *dynsymtab;
    char         *dynstrtab;
    void        **pltgot;
    void         *entry;

    size_t   size;
    intptr_t base;
};

static inline int object_read(struct object *obj, void *dst, size_t count, off_t off) {
    if (fseek(obj->fp, off, SEEK_SET) != 0) {
        return -1;
    }
    if (fread(dst, count, 1, obj->fp) < 1) {
        return -1;
    }
    return 0;
}

//static void object_place(struct object *obj) {
//    Elf64_Phdr phdr;
//    uintptr_t lowest = (uintptr_t) -1;
//    uintptr_t highest = (uintptr_t) -1;
//
//    for (size_t i = 0; i < obj->ehdr.e_phnum; ++i) {
//        off_t off = obj->ehdr.e_phoff + i * obj->ehdr.e_phentsize;
//
//        if (object_read(obj, &phdr, obj->ehdr.e_phentsize, off) != 0) {
//            return;
//        }
//
//        if (phdr.p_type == PT_LOAD) {
//            uintptr_t start = phdr.p_vaddr & ~0xFFF;
//            uintptr_t end = (phdr.p_vaddr + phdr.p_memsz + 0xFFF) & ~0xFFF;
//
//            if (lowest == (uintptr_t) -1 || lowest > start) {
//                lowest = start;
//            }
//            if (highest == (uintptr_t) -1 || highest < end) {
//                highest = end;
//            }
//        }
//    }
//
//    obj->size = highest - lowest;
//    obj->base = -lowest;
//}

struct object *object_open(const char *pathname) {
    ygg_debug_trace("object_open(%s)\n", pathname);
    struct object *obj = calloc(1, sizeof(struct object));
    if (!obj) {
        return NULL;
    }

    if (!(obj->fp = fopen(pathname, "r"))) {
        free(obj);
        return NULL;
    }
    setvbuf(obj->fp, NULL, _IONBF, 0);

    if (object_read(obj, &obj->ehdr, sizeof(Elf64_Ehdr), 0) != 0) {
        fclose(obj->fp);
        free(obj);
        return NULL;
    }

    if (strncmp((const char *) obj->ehdr.e_ident, "\x7F" "ELF", 4)) {
        ygg_debug_trace("bad ELF signature\n");
        fclose(obj->fp);
        free(obj);
        return NULL;
    }

    strcpy(obj->path, pathname);

    return obj;
}

int object_load(struct object *obj) {
    Elf64_Phdr phdr;

    // Load the sections and extract useful locations
    for (size_t i = 0; i < obj->ehdr.e_phnum; ++i) {
        off_t off = obj->ehdr.e_phoff + i * obj->ehdr.e_phentsize;

        if (object_read(obj, &phdr, obj->ehdr.e_phentsize, off) != 0) {
            return -1;
        }

        switch (phdr.p_type) {
        case PT_LOAD:
            if (obj->base == 0) {
                // Not PIC - map segment directly to its vaddr
                uintptr_t start = phdr.p_vaddr & ~0xFFF;
                uintptr_t end = (phdr.p_vaddr + phdr.p_memsz + 0xFFF) & ~0xFFF;
                size_t size = end - start;

                if (mmap((void *) start, size,
                         PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED,
                         -1, 0) == MAP_FAILED) {
                    return -1;
                }
            }

            // Load up to filesz of data
            if (object_read(obj, (void *) (phdr.p_vaddr + obj->base),
                            phdr.p_filesz, phdr.p_offset) != 0) {
                return -1;
            }

            // Zero the rest up to memsz
            if (phdr.p_memsz > phdr.p_filesz) {
                memset((void *) (phdr.p_vaddr + obj->base + phdr.p_filesz),
                       0, phdr.p_memsz - phdr.p_filesz);
            }
            break;
        case PT_DYNAMIC:
            obj->dynamic = (Elf64_Dyn *) (phdr.p_vaddr + obj->base);
            break;
        default:
            ygg_debug_trace("Skipping unknown segment type: %02x\n", phdr.p_type);
            break;
        }
    }

    // Extract dynamic stuff
    if (obj->dynamic) {
        for (size_t i = 0; obj->dynamic[i].d_tag; ++i) {
            Elf64_Dyn *dyn = &obj->dynamic[i];

            switch (dyn->d_tag) {
            case DT_JMPREL:
                obj->jmprela = (Elf64_Rela *) (dyn->d_un.d_ptr + obj->base);
                ygg_debug_trace("object jmprela = %p\n", obj->jmprela);
                break;
            case DT_PLTGOT:
                obj->pltgot = (void **) (dyn->d_un.d_ptr + obj->base);
                ygg_debug_trace("object pltgot = %p\n", obj->pltgot);
                break;
            case DT_SYMTAB:
                obj->dynsymtab = (Elf64_Sym *) (dyn->d_un.d_ptr + obj->base);
                ygg_debug_trace("object dynamic symtab = %p\n", obj->dynsymtab);
                break;
            case DT_STRTAB:
                obj->dynstrtab = (char *) (dyn->d_un.d_ptr + obj->base);
                ygg_debug_trace("object dynamic strtab = %p\n", obj->dynstrtab);
                break;
            case DT_NEEDED:
            case DT_HASH:
                break;
            default:
                ygg_debug_trace("Skipping unknown dynamic tag: %02x\n", dyn->d_tag);
                break;
            }
        }
    }

    // pltgot + 0x10
    obj->pltgot[1] = obj;
    obj->pltgot[2] = object_resolver_entry;
    obj->entry = (void *) (obj->ehdr.e_entry + obj->base);

    // TODO: actual dependency loading
    // TODO: extract exported symbols from libs

    return 0;
}

static int dummy1(int a, int b) {
    return a + b;
}

static int dummy0(int a, int b) {
    return a * b;
}

void *object_resolver(struct object *obj, size_t index) {
    Elf64_Rela *rela = &obj->jmprela[index];
    Elf64_Word sym_index = ELF64_R_SYM(rela->r_info);
    Elf64_Sym *sym = &obj->dynsymtab[sym_index];
    char *name = &obj->dynstrtab[sym->st_name];
    void *res = NULL;

    ygg_debug_trace("%s: lookup %s\n", obj->path, name);

    // TODO: actual lookup
    if (!strcmp(name, "dyn_add")) {
        res = dummy1;
    } else if (!strcmp(name, "dyn_mul")) {
        res = dummy0;
    }

    if (res) {
        // Write the PLT entry
        // Guess they're all R_X86_64_JUMP_SLOTs?
        *(uintptr_t *) (rela->r_offset + obj->base) = (uintptr_t) res;
    }

    return res;
}

object_entry_t object_entry_get(struct object *obj) {
    return obj->entry;
}
