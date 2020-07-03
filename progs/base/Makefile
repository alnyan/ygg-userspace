CC?=$(CROSS_COMPILE)gcc
CFLAGS?=-ggdb \
	   	-O0 \
	   	-Wall \
	   	-Werror \
		-Wno-char-subscripts \
		-Iinclude
LDFLAGS?=-lgcc

O=build

BINS=$(O)/bin/ls \
	 $(O)/bin/pager \
	 $(O)/bin/cat \
	 $(O)/bin/login \
	 $(O)/bin/date \
	 $(O)/bin/grep \
	 $(O)/bin/hexd \
	 $(O)/bin/mkdir \
	 $(O)/bin/rm \
	 $(O)/bin/table \
	 $(O)/bin/uname \
	 $(O)/bin/wr \
	 $(O)/sbin/lspci \
	 $(O)/sbin/insmod \
	 $(O)/sbin/reboot

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
