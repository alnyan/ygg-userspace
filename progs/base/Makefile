CC?=$(CROSS_COMPILE)gcc
CFLAGS?=-ggdb \
	   	-O0 \
	   	-Wall \
	   	-Werror \
		-Wno-format \
		-Wno-char-subscripts \
		-Iinclude
LDFLAGS?=-lgcc

O=build

BINS=$(O)/bin/login \
	 $(O)/bin/pager \
	 $(O)/bin/cat \
	 $(O)/bin/hexd \
	 $(O)/bin/uname \
	 $(O)/bin/ps \
	 $(O)/bin/table \
	 $(O)/bin/grep \
	 $(O)/bin/sleep \
	 $(O)/bin/mkdir \
	 $(O)/sbin/mount \
	 $(O)/sbin/lspci \
	 $(O)/sbin/acpid \
	 $(O)/sbin/reboot \
	 $(O)/bin/date \
	 $(O)/bin/rm \
	 $(O)/bin/ls

#	 $(O)/bin/ucat \
#	 $(O)/bin/mkfifo \
#	 $(O)/sbin/umount \
#	 $(O)/sbin/insmod \

all: mkdirs $(BINS)

mkdirs:
	mkdir -p $(O)/bin $(O)/sbin

clean:
	rm -rf $(O)

install: all
	$(foreach bin,$(BINS),						    		\
		install -m0755 $(bin) $(DESTDIR)/$(bin:$(O)/%=%);	\
	)

$(O)/%: %.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $<
