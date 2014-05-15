
prefix=/usr/local
bindir=$(prefix)/bin
privlibdir=$(prefix)/lib/tuf

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

tuf:
	@true

libtuf.so libtuf.dylib: tuf.o
	$(CC) -shared $(LDFLAGS) $(LDLIBS) $< -o $@

clean:
	rm -rf tuf.o $(lib)

install: $(lib) tuf tuf-deps tuf-test tuf-unused
	mkdir -p ${DESTDIR}${bindir}
	cp -vP tuf tuf-deps tuf-test tuf-unused ${DESTDIR}${bindir}
	
	mkdir -p ${DESTDIR}${privlibdir}
	cp -vP $(lib) ${DESTDIR}${privlibdir}
