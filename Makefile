#
# Makefile
# Adrian Perez, 2012-01-05 19:08
#

PREFIX      ?= /usr/local
CFLAGS      += -Wall
CPPFLAGS    += -DG_DISABLE_DEPRECATED   \
               -DGTK_DISABLE_DEPRECATED \
               -DVTE_DISABLE_DEPRECATED \
               -DPREFIX=\"$(PREFIX)\"
PKG_MODULES := vte-2.90
PKG_CFLAGS  := $(shell pkg-config --cflags $(PKG_MODULES))
PKG_LDFLAGS := $(shell pkg-config --libs   $(PKG_MODULES))

all: dwt dwt.1

dwt: CFLAGS += $(PKG_CFLAGS)
dwt: LDFLAGS += $(PKG_LDFLAGS)
dwt: dwt.o

dwt.1: dwt.rst
	rst2man $< $@

clean:
	$(RM) dwt dwt.o

install: all
	install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 -t $(DESTDIR)$(PREFIX)/bin dwt
	install -m 755 -d $(DESTDIR)$(PREFIX)/man/man1
	install -m 644 -t $(DESTDIR)$(PREFIX)/man/man1 dwt.1

ifeq ($(origin TAG),command line)
VERSION := $(TAG)
else
VERSION := $(shell git tag 2> /dev/null | tail -1)
endif

dist:
ifeq ($(strip $(VERSION)),)
	@echo "ERROR: Either Git is not installed, or no tags were found"
else
	git archive --prefix=dwt-$(VERSION)/ $(VERSION) | xz -c > dwt-$(VERSION).tar.xz
endif

.PHONY: clean install dist

