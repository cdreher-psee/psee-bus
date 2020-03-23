# Makefile to build i2c test for Prophesee systems.
CFLAGS= -Wall
BINARIES= psee-i2c-get psee-i2c-set psee-spi-set
DESTDIR= usr/bin

all: $(BINARIES)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

psee-i2c-get: psee-i2c-get.o
	$(CC) $(LDFLAGS) -o $@ $^

psee-i2c-set: psee-i2c-set.o
	$(CC) $(LDFLAGS) -o $@ $^

psee-spi-set: psee-spi-set.o
	$(CC) $(LDFLAGS) -o $@ $^

install: $(BINARIES)
	install -d $(DESTDIR)
	install -m 0755 psee-i2c-get $(DESTDIR)
	install -m 0755 psee-i2c-set $(DESTDIR)
	install -m 0755 psee-spi-set $(DESTDIR)

clean :
	rm -f $(BINARIES)
	rm -f psee-i2c-get.o psee-i2c-set.o psee-spi-set.o

