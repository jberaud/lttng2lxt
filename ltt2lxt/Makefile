# Makefile for ltt2lxt

PREFIX  ?=/
INSTALL = install
CC 		= gcc
CFLAGS	= -g -Wall -O2
HEADERS = lxt_write.h ltt2lxt.h
LIBS	= -lz -lbz2
PROGRAM	= ltt2lxt

MODULES	= $(patsubst %.c,%.o,$(wildcard mod_*.c))
OBJS	= lxt_write.o ltt2lxt.o atag.o parse.o symbol.o modules.o savefile.o \
		$(MODULES)

all: $(PROGRAM)

%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -Wl,--sort-section,name

clean:
	-rm -f $(OBJS) $(PROGRAM) *~

install: all
	mkdir -p $(DESTDIR)$(PREFIX)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -D $(PROGRAM) $(DESTDIR)$(PREFIX)/bin

install-bin: install
	$(INSTALL) -D bin/gtkwave-parrot $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -D bin/ltt/bin/[^C]* $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	$(INSTALL) -D bin/ltt/lib/*.so.* $(DESTDIR)$(PREFIX)/lib/
	mkdir -p $(DESTDIR)$(PREFIX)/share/lttv/pixmaps
	$(INSTALL) -D bin/ltt/share/lttv/pixmaps/[^C]* $(DESTDIR)$(PREFIX)/share/lttv/pixmaps
	mkdir -p $(DESTDIR)$(PREFIX)/lib/lttv/plugins/
	$(INSTALL) -D bin/ltt/lib/lttv/plugins/[^C]* $(DESTDIR)$(PREFIX)/lib/lttv/plugins/
	sed -i $(DESTDIR)$(PREFIX)/bin/lttv -e "s|^  \$$0.real|LD_LIBRARY_PATH=$(PREFIX)/lib:$(PREFIX)/lib/lttv/plugins &|"

