IDIR =include
CC=gcc
CFLAGS=-g -I$(IDIR) -pthread -Wall

SDIR=src
ODIR=$(SDIR)/obj

LIBS=-lncurses

_DEPS = rat.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = \
	annotate.o \
	buffer.o \
	key_stack.o \
	line_ends.o \
	pager.o \
	poll_registry.o \
	rat.o \
	strbuf.o \
	tokenizer.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rat: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
