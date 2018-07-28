IDIR =include
CC=gcc
CFLAGS=-g -I$(IDIR) -pthread -Wall -lm

SDIR=src
ODIR=$(SDIR)/obj

LIBS=-lncurses

_DEPS = rat.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = \
	annotation.o \
	annotation_parser.o \
	annotations.o \
	annotator.o \
	box.o \
	buffer.o \
	duktape.o \
	js_api.o \
	key_stack.o \
	line_ends.o \
	main.o \
	pager.o \
	pager_stack.o \
	poll_registry.o \
	rat.o \
	read_queue.o \
	strbuf.o \
	tokenizer.o \
	util.o \
	widget.o

OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rat: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
