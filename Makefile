CC ?= gcc

LIB = libmath.so
BIN = demo

all: $(LIB) $(BIN)

$(LIB): libmath.c
	$(CC) -fPIC -shared -o $(LIB) $<

$(BIN): demo.c
	$(CC) -o $(BIN) demo.c -L. -lmath

clean:
	rm -rf *.o *.so* $(BIN)