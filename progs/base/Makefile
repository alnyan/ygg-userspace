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
	 $(O)/sbin/mount \
	 $(O)/sbin/lspci \
	 $(O)/bin/date \
	 $(O)/bin/ls

#	 $(O)/bin/mkdir \
#	 $(O)/bin/rm \
#	 $(O)/bin/test \
#	 $(O)/bin/mkfifo \
#	 $(O)/bin/ucat \
#	 $(O)/sbin/umount \
#	 $(O)/sbin/insmod \
#	 $(O)/sbin/reboot \
#	 $(O)/sbin/acpid

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
