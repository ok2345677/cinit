# cinit - minimal POSIX init for Linux
#
# Builds on any distro (Alpine, Chimera, KISS, Void, Gentoo, ...)
# with gcc or clang. No dependencies beyond libc + Linux headers.
#
# Defaults build a static binary (recommended for init). If your
# distro lacks static libc, run: make STATIC=0

CC     ?= cc
CFLAGS ?= -O2 -Wall -Wextra
STATIC ?= 1

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/sbin

ifeq ($(STATIC),1)
LDFLAGS += -static
endif

all: cinit

cinit: cinit.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

install: cinit
	install -d $(DESTDIR)$(BINDIR)
	install -m755 cinit $(DESTDIR)$(BINDIR)/cinit

clean:
	rm -f cinit

.PHONY: all install clean
