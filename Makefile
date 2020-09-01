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
