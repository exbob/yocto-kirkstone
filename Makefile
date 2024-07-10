CC ?= gcc

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

LIB = libmath.so
BIN = demo

all: $(LIB) $(BIN)

$(LIB): libmath.c
	$(CC) -fPIC -shared -o $(LIB) $<

$(BIN): demo.c
	$(CC) -o $(BIN) demo.c -L. -lmath

install:
	install -d $(LIBDIR)
	install -m 755 $(LIB) $(LIBDIR)
	install -d $(INCLUDEDIR)
	install -m 644 libmath.h $(INCLUDEDIR)
	
clean:
	rm -rf *.o *.so* $(BIN)