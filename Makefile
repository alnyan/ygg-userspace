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
		  $(STAGE)/bin/login

# TODO
# newlib: gettimeofday is broken?

# newlib: port mount()/umount()
# newlib: port reboot()
		 # $(STAGE)/bin/reboot \


		 # $(STAGE)/bin/netctl \
		 # $(STAGE)/bin/netmeow \
		 # $(STAGE)/bin/netdump \
		 # $(STAGE)/bin/ping \

		 # $(STAGE)/bin/mouse \
		 # $(STAGE)/bin/ase \
		 # $(STAGE)/bin/vsh

#		  $(STAGE)/bin/com \
#		  $(STAGE)/bin/su \

sh_OBJS=$(O)/sh/sh.o \
		$(O)/sh/readline.o \
		$(O)/sh/builtin.o \
		$(O)/sh/cmd.o
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

all: mkdirs lua $(O)/initrd.img

clean:
	rm -rf $(O)
	make -j5 -C ../lua clean

lua:
	make -C ../lua yggdrasil
	mkdir -p $(STAGE)/bin
	cp ../lua/lua $(STAGE)/bin/lua

#chmod 04711 $(STAGE)/bin/su
$(O)/initrd.img: mkstage-etc $(STAGE_BIN)
	cd $(STAGE) && tar czf $(abspath $@) *

mkdirs:
	mkdir -p $(DIRS)

mkstage-etc:
	mkdir -p $(STAGE)/dev $(STAGE)/mnt $(STAGE)/bin $(STAGE)/sys
	cp -r etc $(STAGE)

# Application building
$(STAGE)/init: init.c
	@printf " CC\t%s\n" $(@:$(STAGE)/%=/%)
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) init.c

$(STAGE)/bin/%: core/bin/%.c
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) $<

$(STAGE)/bin/sh: $(sh_OBJS)
	$(CC) -o $@ $(usr_LDFLAGS) $(sh_OBJS)

$(STAGE)/bin/vsh: $(vsh_OBJS)
	$(CC) -o $@ $(usr_LDFLAGS) $(vsh_OBJS)

$(STAGE)/bin/ase: $(ase_OBJS)
	$(CC) -o $@ $(usr_LDFLAGS) $(ase_OBJS)

$(O)/vsh/%.o: vsh/%.c $(shell find vsh -name "*.h")
	$(CC) -c -o $@ $(usr_CFLAGS) $<
$(O)/vsh/%.o: vsh/%.S
	$(CC) -c -o $@ $(usr_CFLAGS) $<

$(O)/sh/%.o: sh/%.c $(shell find sh -name "*.h")
	$(CC) -c -o $@ $(usr_CFLAGS) $<

$(O)/ase/%.o: ase/%.c $(shell find ase -name "*.h")
	$(CC) -c -o $@ $(usr_CFLAGS) $<
