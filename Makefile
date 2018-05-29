IDIR =include
CC=gcc
CFLAGS=-I$(IDIR) -pthread

SDIR=src
ODIR=$(SDIR)/obj

LIBS=-lncurses

_DEPS = rat.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = rat.o strbuf.o stream.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rat: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
