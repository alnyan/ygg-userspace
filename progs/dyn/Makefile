CC?=$(CROSS_COMPILE)gcc
O=build

CFLAGS=-Wall \
	   -Werror \
	   -Wextra \
	   -z max-page-size=0x1000 \
	   -Iinclude \
	   -O0

ld_OBJS=$(O)/ldso/ld.o \
		$(O)/ldso/hash.o \
		$(O)/ldso/plt_call.o \
		$(O)/ldso/object.o
HDRS=$(shell find include -type f -name "*.h")

all: mkdirs $(O)/test $(O)/ld.so

mkdirs:
	mkdir -p $(O)/ldso

clean:
	rm -rf $(O)

install: mkdirs $(O)/test $(O)/ld.so
	mkdir -p $(DESTDIR)/lib $(DESTDIR)/bin
	install -D -m 0755 $(O)/ld.so $(DESTDIR)/lib/ld
	install -D -m 0755 $(O)/libfunc.so $(DESTDIR)/lib/
	install -D -m 0755 $(O)/test $(DESTDIR)/bin/dyntest

### ld.so
$(O)/ld.so: $(ld_OBJS) ld-link.ld
	$(CC) $(CFLAGS) -Tld-link.ld -static -o $@ $(ld_OBJS)

$(O)/ldso/%.o: src/%.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(O)/ldso/%.o: src/%.S $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

### Test build
$(O)/libfunc.o: test/lib.c $(HDRS)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

$(O)/libfunc.so: $(O)/libfunc.o
	$(CROSS_COMPILE)ld -shared -o $@ $+

$(O)/test: test/test.c $(HDRS)
	$(CC) $(CFLAGS) -L$(O) -lfunc -nostdlib -fPIC -o $@ test/test.c
