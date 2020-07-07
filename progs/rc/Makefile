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

all: mkdirs $(O)/rc

mkdirs:
	mkdir -p $(O)

clean:
	rm -rf $(O)

install: mkdirs $(O)/rc
	install -m0755 $(O)/rc $(DESTDIR)/init
	# TODO: symlinks in tarfs
	#ln -sf /bin/rc $(DESTDIR)/init

$(O)/rc: $(rc_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(rc_OBJS)

$(O)/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

