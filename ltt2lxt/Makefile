# Makefile for ltt2lxt

PREFIX  ?=/
INSTALL = install
CC 		= gcc
CFLAGS	= -g -Wall -O2
HEADERS = lxt_write.h ltt2lxt.h
LIBS	= -lz -lbz2
PROGRAM_2_6_2X	= ltt2lxt_2_6_2X 
PROGRAM_2_6_3X = ltt2lxt_2_6_3X

MODULES	= $(patsubst %.c,%.o,$(wildcard mod_*.c))
OBJS	= lxt_write.o ltt2lxt.o atag.o parse.o symbol.o modules.o savefile.o \
		$(MODULES)

all: $(PROGRAM_2_6_2X) $(PROGRAM_2_6_3X)

2_6_2X/%.o : %.c $(HEADERS) 2_6_2X/.dir
	$(CC) $(CFLAGS) -c $< -o $@

2_6_3X/%.o : %.c $(HEADERS) 2_6_3X/.dir
	$(CC) $(CFLAGS) -DARCH_2_6_3X -c $< -o $@

2_6_3X/.dir:
	mkdir -p $(dir $@)
	touch $@

2_6_2X/.dir:
	mkdir -p $(dir $@)
	touch $@

$(PROGRAM_2_6_2X): $(addprefix 2_6_2X/,$(OBJS))
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS) -Wl,--sort-section,name

$(PROGRAM_2_6_3X) : $(addprefix 2_6_3X/,$(OBJS))
	$(CC) -DARCH_2_6_3X $(CFLAGS) -o $@ $^ $(LIBS) -Wl,--sort-section,name

clean:
	-rm -f $(addprefix 2_6_2X/,$(OBJS)) $(addprefix 2_6_3X/,$(OBJS)) $(PROGRAM_2_6_2X) $(PROGRAM_2_6_3X) *~

install: all
	mkdir -p $(DESTDIR)$(PREFIX)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -D $(PROGRAM_2_6_2X) $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -D $(PROGRAM_2_6_3X) $(DESTDIR)$(PREFIX)/bin

install-bin: install
	$(INSTALL) -D bin/gtkwave-parrot $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -D bin/ltt_2_6_2X/bin/[!C]* $(DESTDIR)$(PREFIX)/bin/
	$(INSTALL) -D bin/ltt_2_6_3X/bin/[!C]* $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/lib_2_6_2X
	mkdir -p $(DESTDIR)$(PREFIX)/lib_2_6_3X
	$(INSTALL) -D bin/ltt_2_6_2X/lib/*.so.* $(DESTDIR)$(PREFIX)/lib_2_6_2X/
	$(INSTALL) -D bin/ltt_2_6_3X/lib/*.so* $(DESTDIR)$(PREFIX)/lib_2_6_3X/
	mkdir -p $(DESTDIR)$(PREFIX)/lib_2_6_2X/lttv/plugins/
	mkdir -p $(DESTDIR)$(PREFIX)/lib_2_6_3X/lttv/plugins/
	$(INSTALL) -D bin/ltt_2_6_2X/lib/lttv/plugins/[!C]* $(DESTDIR)$(PREFIX)/lib_2_6_2X/lttv/plugins/
	$(INSTALL) -D bin/ltt_2_6_3X/lib/lttv/plugins/[!C]* $(DESTDIR)$(PREFIX)/lib_2_6_3X/lttv/plugins/
	sed -i $(DESTDIR)$(PREFIX)/bin/lttv_2_6_2X -e "s|^  \$$0.real|LD_LIBRARY_PATH=$(PREFIX)/lib:$(PREFIX)/lib_2_6_2X/lttv/plugins &|"
	sed -i $(DESTDIR)$(PREFIX)/bin/lttv_2_6_3X -e "s|^  \$$0.real|LD_LIBRARY_PATH=$(PREFIX)/lib_2_6_3X:$(PREFIX)/lib_2_6_3X/lttv/plugins &|"

