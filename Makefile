CFLAGS = -I ./ -std=c99 -g -Wall


all: hello deatr

1.0: 1.h 1.c
	$(CC) -$< -o $@

hello: 1.o hello.o 1.h
	$(CC) $(CFLAGS) 1.o hello.o -o $@

deatr: 1.o deatr.o 1.h
	$(CC) $(CFLAGS) 1.o deatr.o -o $@



clean:
	rm -rf *.o hello deatr


# Installation

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

1.a: 1.o
	ar -q 1.a 1.o

install: 1.h 1.a
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -m 644 1.a $(DESTDIR)$(PREFIX)/lib/
	install -m 644 1.h $(DESTDIR)$(PREFIX)/include/
