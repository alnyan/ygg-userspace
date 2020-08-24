O?=build
STAGE=$(O)/stage
KERNEL_HDRS?=kernel-hdr

CC=x86_64-elf-yggdrasil-gcc

DIRS=$(STAGE)

all: mkdirs make-image

make-image: $(O)/initrd.img

clean:
	rm -rf $(O)
	$(foreach dir,$(wildcard progs/*), 	\
		make -C $(dir) clean;			\
	)

$(O)/initrd.img: mkstage-etc mkstage-progs
	@cd $(STAGE) && tar cf $(abspath $@) *

mkdirs:
	@mkdir -p $(DIRS)

mkstage-etc:
	@mkdir -p $(STAGE)/dev \
			 $(STAGE)/mnt \
			 $(STAGE)/bin \
			 $(STAGE)/lib \
			 $(STAGE)/sys \
			 $(STAGE)/sbin \
			 $(STAGE)/tmp
	@cp -r etc $(STAGE)

mkstage-progs:
	@$(foreach dir,$(wildcard progs/*), 											\
		CC=$(CC) DESTDIR=$(abspath $(STAGE)) make -C $(dir) install || exit 1;	\
	)
