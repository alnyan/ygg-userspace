O?=build
STAGE=$(O)/stage
KERNEL_HDRS?=kernel-hdr

CC=x86_64-elf-yggdrasil-gcc

DIRS=$(O) \
	 $(STAGE)
HDRS=$(shell find $(S) -type f -name "*.h")
STAGE_BIN=$(STAGE)/init \
		  $(STAGE)/bin/hexd \
		  $(STAGE)/bin/ls \
		  $(STAGE)/bin/reboot

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
