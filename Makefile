os := $(shell uname)

CFLAGS=-fPIC
ifeq ($(os),FreeBSD)
LDLIBS=
else
LDLIBS=-ldl
endif

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

