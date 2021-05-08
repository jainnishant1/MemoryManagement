IDIR=include
CC=gcc
CFLAGS=-lrt -lpthread

ODIR=obj

DEPS = mgr.h heap.h memmgr.h cli.h
# DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = mgr.o heap.o test.o cli.o
# OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core *~ 