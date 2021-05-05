IDIR=include
CC=gcc
CFLAGS=

ODIR=obj

DEPS = mgr.h memmgr.h
# DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

OBJ = mgr.o test.o
# OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

test: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~ core *~ 