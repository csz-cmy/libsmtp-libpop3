GCC=gcc
GCCFLAGS=-Wall -Wextra -pedantic
OBJ=libsmtp.o
BIN=libsmtp.a

all: $(BIN)

libsmtp.a: $(OBJ)
	ar -r $(BIN) $(OBJ)

libsmtp.o: libsmtp.c
	$(GCC) $(GCCFLAGS) -c libsmtp.c

clean:
	rm -f $(OBJ)
	rm -f $(BIN)

.PHONY: clean
