IDIR =include
CC=gcc
CFLAGS=-g -I$(IDIR) -pthread

SDIR=src
ODIR=$(SDIR)/obj

LIBS=-lncurses

_DEPS = rat.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = \
	buffer.o \
	input_queue.o \
	key_stack.o \
	lineends.o \
	pager.o \
	rat.o \
	strbuf.o \
	stream.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rat: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
