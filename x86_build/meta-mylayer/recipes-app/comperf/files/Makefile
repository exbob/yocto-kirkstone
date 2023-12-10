comperf:comperf.c
	$(CC) ${CFLAGS} $< -o $@ -lrt $(LDFLAGS)

install:
	install -d $(DESTDIR)
	install -m 0755 comperf $(DESTDIR)

clean:
	rm -rf comperf