# Makefile for ltt2lxt

PREFIX  ?=/
INSTALL = install
CC 		= gcc
CFLAGS	= -g -Wall -O2
HEADERS = lxt_write.h ltt2lxt.h
LIBS	= -lz -lbz2
PROGRAM	= ltt2lxt 
PROGRAM_OMAP = ltt2lxt_omap

MODULES	= $(patsubst %.c,%.o,$(wildcard mod_*.c))
OBJS	= lxt_write.o ltt2lxt.o atag.o parse.o symbol.o modules.o savefile.o \
		$(MODULES)

all: $(PROGRAM) $(PROGRAM_OMAP)

p6/%.o : %.c $(HEADERS) p6
	$(CC) $(CFLAGS) -c $< -o $@

omap/%.o : %.c $(HEADERS) omap
	$(CC) $(CFLAGS) -DARCH_OMAP -c $< -o $@

omap:
	mkdir -p $@

p6: 
	mkdir -p $@

$(PROGRAM): $(addprefix p6/,$(OBJS))
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -Wl,--sort-section,name

$(PROGRAM_OMAP) : $(addprefix omap/,$(OBJS))
	$(CC) -DARCH_OMAP $(CFLAGS) -o $@ $^ $(LIBS) -Wl,--sort-section,name

clean:
	-rm -f $(addprefix p6/,$(OBJS)) $(addprefix omap/,$(OBJS)) $(PROGRAM) $(PROGRAM_OMAP) *~

install: all
	mkdir -p $(DESTDIR)$(PREFIX)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -D $(PROGRAM) $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -D $(PROGRAM_OMAP) $(DESTDIR)$(PREFIX)/bin

install-bin: install
	$(INSTALL) -D bin/gtkwave-parrot $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -D bin/ltt/bin/[!C]* $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -D bin/ltt-2.6.32/bin/[!C]* $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/lib-2.6.32
	$(INSTALL) -D bin/ltt/lib/*.so.* $(DESTDIR)$(PREFIX)/lib/
	$(INSTALL) -D bin/ltt-2.6.32/lib/*.so* $(DESTDIR)$(PREFIX)/lib-2.6.32/
	mkdir -p $(DESTDIR)$(PREFIX)/lib/lttv/plugins/
	mkdir -p $(DESTDIR)$(PREFIX)/lib-2.6.32/lttv/plugins/
	$(INSTALL) -D bin/ltt/lib/lttv/plugins/[!C]* $(DESTDIR)$(PREFIX)/lib/lttv/plugins/
	$(INSTALL) -D bin/ltt-2.6.32/lib/lttv/plugins/[!C]* $(DESTDIR)$(PREFIX)/lib-2.6.32/lttv/plugins/
	sed -i $(DESTDIR)$(PREFIX)/bin/lttv -e "s|^  \$$0.real|LD_LIBRARY_PATH=$(PREFIX)/lib:$(PREFIX)/lib/lttv/plugins &|"
	sed -i $(DESTDIR)$(PREFIX)/bin/lttv-2.6.32 -e "s|^  \$$0.real|LD_LIBRARY_PATH=$(PREFIX)/lib-2.6.32:$(PREFIX)/lib-2.6.32/lttv/plugins &|"

