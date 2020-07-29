CC?=$(CROSS_COMPILE)gcc
O=build

all: mkdirs $(O)/dyn $(O)/ld

mkdirs:
	mkdir -p $(O)

clean:
	rm -rf $(O)

install: mkdirs $(O)/dyn $(O)/ld
	mkdir -p $(DESTDIR)/lib $(DESTDIR)/bin
	install -D -m 0755 $(O)/dyn $(DESTDIR)/bin/
	install -D -m 0755 $(O)/libfunc.so $(DESTDIR)/lib/
	install -D -m 0755 $(O)/ld $(DESTDIR)/lib/

$(O)/ld: ld.c
	$(CC) -static -o $@ $+

$(O)/libfunc.o: lib.c
	$(CC) -fPIC -c -o $@ $<

$(O)/libfunc.so: $(O)/libfunc.o
	$(CROSS_COMPILE)ld -shared -o $@ $+

$(O)/dyn: dyn.c $(O)/libfunc.so
	$(CC) -L$(O) -lfunc -o $@ dyn.c
