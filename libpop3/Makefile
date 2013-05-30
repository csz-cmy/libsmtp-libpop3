GCC=gcc
GCCFLAGS=-Wall -Wextra -pedantic
OBJ=libpop3.o
BIN=libpop3.a

all: $(BIN)

libpop3.a: $(OBJ)
	ar -r $(BIN) $(OBJ)

libpop3.o: libpop3.c
	$(GCC) $(GCCFLAGS) -c libpop3.c

clean:
	rm -f $(OBJ)
	rm -f $(BIN)

.PHONY: clean
