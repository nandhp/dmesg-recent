CFLAGS=-g -Wall -O3
PREFIX=/usr/local

all: dmesg-recent
clean:
	rm -f dmesg-recent *.o
install:
	install -t $(PREFIX)/bin dmesg-recent
