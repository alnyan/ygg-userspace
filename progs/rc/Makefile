CC?=$(CROSS_COMPILE)gcc
CFLAGS?=-ggdb \
	   	-O0 \
	   	-Wall \
	   	-Werror \
		-Wno-char-subscripts \
		-Iinclude
LDFLAGS?=-lgcc

O=build

rc_OBJS=$(O)/rc.o
init_OBJS=$(O)/init.o

all: mkdirs $(O)/init $(O)/rc

mkdirs:
	mkdir -p $(O)

clean:
	rm -rf $(O)

install: mkdirs $(O)/init $(O)/rc
	install -m0755 $(O)/init $(DESTDIR)/init
	install -m0755 $(O)/rc $(DESTDIR)/sbin/rc

$(O)/rc: $(rc_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(rc_OBJS)

$(O)/init: $(init_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(init_OBJS)

$(O)/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

