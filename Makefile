# Makefile to build i2c test for Prophesee systems.
CFLAGS= -Wall
BINARIES= psee-i2c-get
DESTDIR= usr/bin

all: $(BINARIES)

psee-i2c-get:
	$(CC) $(CFLAGS) -o $@ psee-i2c-get.c

install: $(BINARIES)
	install -d $(DESTDIR)
	install -m 0755 psee-i2c-get $(DESTDIR)

clean :
	rm -f $(BINARIES)

