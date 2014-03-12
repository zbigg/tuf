CFLAGS=-fPIC
LDLIBS=-ldl

os := $(shell uname)

ifeq ($(os),Darwin)
lib = libtuf.dylib
else
lib = libtuf.so
endif

default: $(lib)

libtuf.so libtuf.dylib: tuf.o
	$(CC) -shared $(LDFLAGS) $(LDLIBS) $< -o $@

clean:
	rm -rf tuf.o $(lib)

