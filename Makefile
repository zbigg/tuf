CFLAGS=-fPIC
LDLIBS=-ldl
libtuf.so: tuf.o
	$(CC) -shared $(LDFLAGS) $(LDLIBS) $< -o $@

clean:
	rm -rf tuf.o libtuf.so

