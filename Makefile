#
# Makefile
# Adrian Perez, 2012-01-05 19:08
#

PREFIX      ?= /usr/local
CFLAGS      += -Wall
CPPFLAGS    += -DG_DISABLE_DEPRECATED   \
               -DGTK_DISABLE_DEPRECATED \
               -DVTE_DISABLE_DEPRECATED
PKG_MODULES := vte-2.90
PKG_CFLAGS  := $(shell pkg-config --cflags $(PKG_MODULES))
PKG_LDFLAGS := $(shell pkg-config --libs   $(PKG_MODULES))

all: dwt

dwt: CFLAGS += $(PKG_CFLAGS)
dwt: LDFLAGS += $(PKG_LDFLAGS)
dwt: dwt.o

clean:
	$(RM) dwt dwt.o

install: all
	install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 -t $(DESTDIR)$(PREFIX)/bin dwt

.PHONY: clean install

