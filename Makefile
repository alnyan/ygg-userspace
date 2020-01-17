O?=build
STAGE=$(O)/stage
KERNEL_HDRS?=kernel-hdr

CC=x86_64-elf-yggdrasil-gcc

DIRS=$(O) \
	 $(STAGE) \
	 $(O)/sh
HDRS=$(shell find $(S) -type f -name "*.h")
STAGE_BIN=$(STAGE)/init \
		  $(STAGE)/bin/hexd \
		  $(STAGE)/bin/ls \
		  $(STAGE)/bin/reboot \
		  $(STAGE)/bin/date \
		  $(STAGE)/bin/uname \
		  $(STAGE)/bin/mount \
		  $(STAGE)/bin/umount \
		  $(STAGE)/bin/sh \
		  $(STAGE)/bin/rm \
		  $(STAGE)/bin/mkdir \
		  $(STAGE)/bin/login
sh_OBJS=$(O)/sh/sh.o \
		$(O)/sh/readline.o \
		$(O)/sh/builtin.o \
		$(O)/sh/cmd.o

usr_CFLAGS=-msse \
	   	   -msse2 \
	   	   -ggdb \
	   	   -O0 \
		   -Wall \
		   -Werror \
		   -ffreestanding

usr_LDFLAGS=-lgcc \
			-static

all: mkdirs $(O)/initrd.img

clean:
	rm -rf $(O)

$(O)/initrd.img: mkstage-etc $(STAGE_BIN)
	cd $(STAGE) && tar czf $(abspath $@) *

mkdirs:
	mkdir -p $(DIRS)

mkstage-etc:
	mkdir -p $(STAGE)/dev $(STAGE)/mnt $(STAGE)/bin
	cp -r etc $(STAGE)

# Application building
$(STAGE)/init: init.c
	@printf " CC\t%s\n" $(@:$(STAGE)/%=/%)
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) init.c

$(STAGE)/bin/%: core/bin/%.c
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) $<

$(STAGE)/bin/sh: $(sh_OBJS)
	$(CC) -o $@ $(usr_LDFLAGS) $(sh_OBJS)

$(O)/sh/%.o: sh/%.c $(shell find sh -name "*.h")
	$(CC) -c -o $@ $(usr_CFLAGS) $<
