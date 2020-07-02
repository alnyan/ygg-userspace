O?=build
STAGE=$(O)/stage
KERNEL_HDRS?=kernel-hdr

CC=x86_64-elf-yggdrasil-gcc

DIRS=$(O) \
	 $(STAGE) \
	 $(O)/sh \
	 $(O)/ase \
	 $(O)/vsh
HDRS=$(shell find $(S) -type f -name "*.h")
STAGE_BIN=$(STAGE)/init \
		  $(STAGE)/bin/ls \
		  $(STAGE)/bin/sh \
		  $(STAGE)/bin/hexd \
		  $(STAGE)/bin/uname \
		  $(STAGE)/bin/rm \
		  $(STAGE)/bin/mkdir \
		  $(STAGE)/bin/mount \
		  $(STAGE)/bin/umount \
		  $(STAGE)/bin/date \
		  $(STAGE)/bin/login \
		  $(STAGE)/bin/netctl \
		  $(STAGE)/bin/netdump \
		  $(STAGE)/bin/netmeow \
		  $(STAGE)/bin/ping \
		  $(STAGE)/bin/com \
		  $(STAGE)/bin/wr \
		  $(STAGE)/sbin/insmod \
		  $(STAGE)/sbin/reboot \
		  $(STAGE)/sbin/lspci \
		  $(STAGE)/bin/vsh \
		  $(STAGE)/bin/grep \
		  $(STAGE)/test.ko

#		  $(STAGE)/bin/t0 \

# TODO
# newlib: gettimeofday is broken?

# newlib: port mount()/umount()
# newlib: port reboot()

#		  $(STAGE)/bin/su \




sh_OBJS=$(O)/sh/sh.o \
		$(O)/sh/readline.o \
		$(O)/sh/cmd.o \
		$(O)/sh/parse.o \
		$(O)/sh/builtin.o

ase_OBJS=$(O)/ase/ase.o

vsh_OBJS=$(O)/vsh/vsh.o \
		 $(O)/vsh/input.o \
		 $(O)/vsh/video.o \
		 $(O)/vsh/font.o \
		 $(O)/vsh/logo.o

usr_CFLAGS=-ggdb \
		   -msse \
		   -msse2 \
	   	   -O0 \
		   -Wall \
		   -Werror \
		   -Wno-char-subscripts \
		   -ffreestanding

usr_LDFLAGS=-lgcc \
			-static

all: mkdirs $(O)/initrd.img

clean:
	rm -rf $(O)

#chmod 04711 $(STAGE)/bin/su
$(O)/initrd.img: mkstage-etc $(STAGE_BIN)
	cd $(STAGE) && tar czf $(abspath $@) *

mkdirs:
	mkdir -p $(DIRS)

mkstage-etc:
	mkdir -p $(STAGE)/dev \
			 $(STAGE)/mnt \
			 $(STAGE)/bin \
			 $(STAGE)/sys \
			 $(STAGE)/sbin
	cp -r etc $(STAGE)

# Application building
$(STAGE)/init: init.c
	@printf " CC\t%s\n" $(@:$(STAGE)/%=/%)
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) init.c

$(STAGE)/bin/t0: t0.c
	$(CC) -I../ports/SDL2-2.0.12/include \
		  -o $@ \
		  $(usr_CFLAGS) \
		  $(usr_LDFLAGS) \
		  $< \
		  ../ports/SDL-build/build/.libs/libSDL2.a \
		  -lm

$(STAGE)/bin/%: core/bin/%.c
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) $<

$(STAGE)/sbin/%: core/sbin/%.c
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) $<

$(STAGE)/bin/sh: $(sh_OBJS)
	$(CC) -o $@ $(usr_LDFLAGS) $(sh_OBJS)

$(STAGE)/bin/vsh: $(vsh_OBJS)
	$(CC) -o $@ $(usr_LDFLAGS) $(vsh_OBJS)

$(STAGE)/bin/ase: $(ase_OBJS)
	$(CC) -o $@ $(usr_LDFLAGS) $(ase_OBJS)

$(STAGE)/test.ko: module.c module.ld
	$(CC) \
		-I ../kernel/include \
		-r \
		-O0 \
		-g \
		-mcmodel=large \
		-Tmodule.ld \
		-nostdlib \
		-ffreestanding \
		-o $@ module.c

$(O)/vsh/%.o: vsh/%.c $(shell find vsh -name "*.h")
	$(CC) -c -o $@ $(usr_CFLAGS) $<
$(O)/vsh/%.o: vsh/%.S
	$(CC) -c -o $@ $(usr_CFLAGS) $<

$(O)/sh/%.o: sh/%.c $(shell find sh -name "*.h")
	$(CC) -c -o $@ $(usr_CFLAGS) $<

$(O)/ase/%.o: ase/%.c $(shell find ase -name "*.h")
	$(CC) -c -o $@ $(usr_CFLAGS) $<
