O?=build
STAGE=$(O)/stage
KERNEL_HDRS?=kernel-hdr

CC=x86_64-elf-yggdrasil-gcc

DIRS=$(STAGE)
STAGE_BIN=$(STAGE)/init

all: mkdirs $(O)/initrd.img

clean:
	rm -rf $(O)
	$(foreach dir,$(wildcard progs/*), 	\
		make -C $(dir) clean;			\
	)

$(O)/initrd.img: mkstage-etc $(STAGE_BIN) mkstage-progs
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

mkstage-progs:
	$(foreach dir,$(wildcard progs/*), 									\
		CC=$(CC) DESTDIR=$(abspath $(STAGE)) make -C $(dir) install;	\
	)

$(STAGE)/init: init.c
	$(CC) -o $@ $(usr_CFLAGS) $(usr_LDFLAGS) init.c
