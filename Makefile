O?=build
STAGE=$(O)/stage
KERNEL_HDRS?=kernel-hdr

CC=$(CROSS_COMPILE)gcc

DIRS=$(O) \
	 $(O)/libc \
	 $(STAGE)
HDRS=$(shell find $(S) -type f -name "*.h")
STAGE_BIN=$(STAGE)/init \
		  $(STAGE)/bin/hexd \
		  $(STAGE)/bin/ls \
		  $(STAGE)/bin/reboot

CFLAGS=-ffreestanding \
	   -nostdlib \
	   -msse \
	   -msse2 \
	   -Ilibc/include \
	   -ggdb \
	   -O0 \
	   -I$(KERNEL_HDRS)

usr_LDFLAGS=-nostdlib \
			-lgcc \
			-Tlibc/program.ld
usr_STATIC_LIBS=$(O)/libc.a

libc_CRTI=$(O)/libc/crti.o
libc_CRTN=$(O)/libc/crtn.o
libc_OBJS=$(O)/libc/crt0.o \
		  $(O)/libc/syscall.o \
		  $(O)/libc/vsnprintf.o \
		  $(O)/libc/printf.o \
		  $(O)/libc/string.o \
		  $(O)/libc/errno.o \
		  $(O)/libc/stdio.o \
		  $(O)/libc/init.o \
		  $(O)/libc/malloc.o \
		  $(O)/libc/dirent.o \
		  $(O)/libc/signal.o \
		  $(O)/libc/global.o \
		  $(O)/libc/time.o

sys_CRTBEGIN=$(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
sys_CRTEND=$(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)

all: mkdirs $(O)/initrd.img

clean:
	rm -rf $(O)

$(O)/initrd.img: mkstage-etc $(STAGE_BIN)
	cd $(STAGE) && tar czf $(abspath $@) *

mkdirs:
	mkdir -p $(DIRS)

mkstage-etc:
	mkdir -p $(STAGE)/dev $(STAGE)/mnt $(STAGE)/bin
	chmod +x $(STAGE)/etc/file.txt
	cp -r etc $(STAGE)

# Application building
$(STAGE)/init: init.c $(libc_CRTI) $(libc_CRTN) $(O)/libc.a
	@printf " CC\t%s\n" $(@:$(STAGE)/%=/%)
	@$(CC) -o $@ $(CFLAGS) $(usr_LDFLAGS) \
		$(libc_CRTI) \
		$(sys_CRTBEGIN) \
		init.c \
		$(sys_CRTEND) \
		$(libc_CRTN) \
		$(usr_STATIC_LIBS)

$(STAGE)/bin/%: core/bin/%.c $(libc_CRTI) $(libc_CRTN) $(O)/libc.a
	@printf " CC\t%s\n" $(@:$(STAGE)/%=/%)
	@$(CC) -o $@ $(CFLAGS) $(usr_LDFLAGS) \
		$(libc_CRTI) \
		$(sys_CRTBEGIN) \
		$< \
		$(sys_CRTEND) \
		$(libc_CRTN) \
		$(usr_STATIC_LIBS)

# libc building
$(O)/libc.a: $(libc_OBJS)
	@printf " AR\t%s\n" $(@:$(O)/%=%)
	@$(AR) rcs $@ $(libc_OBJS)

$(O)/libc/%.o: libc/%.c $(HDRS)
	@printf " CC\t%s\n" $(@:$(O)/%=%)
	@$(CC) -c -o $@ $(CFLAGS) $<

$(O)/libc/%.o: libc/%.S
	@printf " AS\t%s\n" $(@:$(O)/%=%)
	@$(CC) -c -o $@ $(CFLAGS) $<
