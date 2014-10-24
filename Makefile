#
# Makefile
# Adrian Perez, 2012-01-05 19:08
#

PREFIX      ?= /usr/local
CFLAGS      += -Wall -std=gnu99 $(EXTRA_CFLAGS)
CPPFLAGS    += -DG_DISABLE_DEPRECATED   \
               -DGTK_DISABLE_DEPRECATED \
               -DVTE_DISABLE_DEPRECATED \
               -DPREFIX=\"$(PREFIX)\"
PKG_MODULES := vte-2.91
PKG_CFLAGS  := $(shell pkg-config --cflags $(PKG_MODULES))
PKG_LDLIBS  := $(shell pkg-config --libs   $(PKG_MODULES))

all: dwt dwt.1 dwt.desktop

dwt: CFLAGS += $(PKG_CFLAGS)
dwt: LDLIBS += $(PKG_LDLIBS)
dwt: dwt.o dwt-settings.o dg-settings.o dwt.gresources.o

%: %.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

dwt.1: dwt.rst
	rst2man $< $@

dwt.gresources.o: menus.xml

%.gresources.c: %.gresources.xml
	glib-compile-resources --generate-source --target=$@ $<

%.gresources.h: %.gresources.xml
	glib-compile-resources --generate-header --target=$@ $<

clean:
	$(RM) dwt dwt.gresources.o dwt.gresources.h dwt.gresources.c dwt.o dg-settings.o dwt-settings.o

install: all
	install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 -t $(DESTDIR)$(PREFIX)/bin dwt
	install -m 755 -d $(DESTDIR)$(PREFIX)/man/man1
	install -m 644 -t $(DESTDIR)$(PREFIX)/man/man1 dwt.1
	install -m 755 -d $(DESTDIR)$(PREFIX)/share/applications
	install -m 644 -t $(DESTDIR)$(PREFIX)/share/applications dwt.desktop

tests: CFLAGS += $(PKG_CFLAGS)
tests: LDLIBS += $(PKG_LDLIBS)
tests: tests/test-settings
tests/test-settings: tests/test-settings.c dg-settings.c

.PHONY: tests

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

print-flags:
	@echo "$(PKG_CFLAGS) $(CPPFLAGS)"

.PHONY: clean install dist print-flags

