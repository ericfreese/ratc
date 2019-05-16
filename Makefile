IDIR =include

CC=gcc
CFLAGS=-g -I$(IDIR) -pthread -Wall -lm

SDIR=src
ODIR=$(SDIR)/obj

LIBS=-lncurses -lutf8proc

_DEPS = rat.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

CFILES := $(wildcard $(SDIR)/*.c)
TESTS := $(filter %_test.c,$(CFILES))
SOURCES := $(filter-out $(TESTS),$(CFILES))
OBJ := $(patsubst $(SDIR)/%.c,$(ODIR)/%.o,$(SOURCES))

%_test: $(SDIR)/%_test.c $(filter-out %main.o,$(OBJ))
	$(CC) $< -lrt -lm -o $@ $(wordlist 2,$(words $^),$^) $(CFLAGS) $(LIBS)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

rat: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: test\:% test clean

test\:%: %_test
	$<

test: $(patsubst $(SDIR)/%.c,%,$(TESTS))
	for t in $^; do echo $$t; $$t; done

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
