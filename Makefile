#
# Makefile
# Adrian Perez, 2012-01-05 19:08
#

PKG_MODULES := vte-2.90
PKG_CFLAGS  := $(shell pkg-config --cflags $(PKG_MODULES))
PKG_LDFLAGS := $(shell pkg-config --libs   $(PKG_MODULES))

all: dwt

dwt: CFLAGS += $(PKG_CFLAGS)
dwt: LDFLAGS += $(PKG_LDFLAGS)
dwt: dwt.o

clean:
	$(RM) dwt dwt.o

.PHONY: clean

