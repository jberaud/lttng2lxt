# Makefile for ltt2lxt

CC 		= gcc
CFLAGS	= -g -Wall -O2
HEADERS = lxt_write.h
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
