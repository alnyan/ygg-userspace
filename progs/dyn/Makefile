CC?=$(CROSS_COMPILE)gcc
O=build

CFLAGS=-Wall \
	   -Werror \
	   -Wextra \
	   -z max-page-size=0x1000 \
	   -O0

all: mkdirs $(O)/dyn $(O)/test $(O)/ld

mkdirs:
	mkdir -p $(O)

clean:
	rm -rf $(O)

install: mkdirs $(O)/dyn $(O)/test $(O)/ld
	mkdir -p $(DESTDIR)/lib $(DESTDIR)/bin
	install -D -m 0755 $(O)/dyn $(DESTDIR)/bin/
	install -D -m 0755 $(O)/libfunc.so $(DESTDIR)/lib/
	install -D -m 0755 $(O)/ld $(DESTDIR)/lib/
	install -D -m 0755 $(O)/test $(DESTDIR)/bin/dyntest

$(O)/ld: ld.c ld-link.ld
	$(CC) $(CFLAGS) -Tld-link.ld -static -o $@ ld.c

$(O)/libfunc.o: lib.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(O)/libfunc.so: $(O)/libfunc.o
	$(CROSS_COMPILE)ld -shared -o $@ $+

$(O)/test: test.c
	$(CC) $(CFLAGS) -nostdlib -o $@ test.c

$(O)/dyn: dyn.c $(O)/libfunc.so
	$(CC) $(CFLAGS) -L$(O) -lfunc -o $@ dyn.c
