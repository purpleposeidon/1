CFLAGS = -I ./ -std=c99 -g -Wall


all: lib1.a hello deatr

1.h: 1_sys.h

hello: 1.o hello.o 1.h 1_sys.h
	$(CC) $(CFLAGS) 1.o hello.o -o $@

deatr: 1.o deatr.o 1.h 1_sys.h
	$(CC) $(CFLAGS) 1.o deatr.o -o $@

lib1.a: 1.o
	ar -r lib1.a 1.o



clean:
	rm -rf *.o hello deatr



# Installation

ifeq ($(PREFIX),)
    PREFIX := /usr/local
endif

install: 1.h lib1.a
	install -d $(DESTDIR)$(PREFIX)/lib/
	install -d $(DESTDIR)$(PREFIX)/include/
	install -d $(DESTDIR)$(PREFIX)/lib/pkgconfig/
	install 1.h $(DESTDIR)$(PREFIX)/include/
	install 1_sys.h $(DESTDIR)$(PREFIX)/include/
	install 1forbidstd.h $(DESTDIR)$(PREFIX)/include/
	install lib1.a $(DESTDIR)$(PREFIX)/lib/
	install 1.pc $(DESTDIR)$(PREFIX)/lib/pkgconfig/
	ldconfig

uninstall:
	rm $(DESTDIR)$(PREFIX)/lib/lib1.a
	rm $(DESTDIR)$(PREFIX)/include/1.h
	rm $(DESTDIR)$(PREFIX)/include/1_sys.h
	rm $(DESTDIR)$(PREFIX)/include/1forbidstd.h
