CC?=$(CROSS_COMPILE)gcc
CFLAGS?=-ggdb \
	   	-O0 \
	   	-Wall \
	   	-Werror \
		-Wno-format \
		-Wno-char-subscripts \
		-Iinclude
LDFLAGS?=-lgcc

bin_zcat_FLAGS=-I lib/zlib-1.2.11
bin_zcat_LIBS=$(O)/lib/zlib/libz.a

S=$(abspath .)
O=build

BINS=$(O)/bin/pager \
	 $(O)/bin/cat \
	 $(O)/bin/hexd \
	 $(O)/bin/uname \
	 $(O)/bin/ps \
	 $(O)/bin/table \
	 $(O)/bin/grep \
	 $(O)/bin/sleep \
	 $(O)/bin/mkdir \
	 $(O)/bin/head \
	 $(O)/bin/zcat \
	 $(O)/bin/showkey \
	 $(O)/bin/com \
	 $(O)/bin/tee \
	 $(O)/sbin/login \
	 $(O)/sbin/mount \
	 $(O)/sbin/lspci \
	 $(O)/sbin/acpid \
	 $(O)/sbin/reboot \
	 $(O)/sbin/insmod \
	 $(O)/bin/date \
	 $(O)/bin/rm \
	 $(O)/bin/cp \
	 $(O)/bin/ls

#	 $(O)/bin/ucat \
#	 $(O)/bin/mkfifo \
#	 $(O)/sbin/umount \

all: mkdirs libs $(BINS)

libs:
	if [ ! -f $(O)/lib/zlib/Makefile ]; then \
		cd $(O)/lib/zlib && CC=$(CROSS_COMPILE)gcc AR=$(CROSS_COMPILE)ar $(S)/lib/zlib-1.2.11/configure; \
	fi
	make -C $(O)/lib/zlib

mkdirs:
	mkdir -p $(O)/bin $(O)/sbin $(O)/lib/zlib

clean:
	rm -rf $(O)

install: all
	$(foreach bin,$(BINS),						    		\
		install -m0755 $(bin) $(DESTDIR)/$(bin:$(O)/%=%);	\
	)
	ln -sf /bin/pager $(DESTDIR)/bin/less
	ln -sf /bin/pager $(DESTDIR)/bin/more

$(O)/%: %.c
	@$(CC) $($(subst /,_,$(@:$(O)/%=%))_FLAGS) \
		$(LDFLAGS) $(CFLAGS) -o $@ \
		$< \
		$($(subst /,_,$(@:$(O)/%=%))_LIBS)
