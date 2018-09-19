CC=gcc
CFLAGS=-Wall -Wextra -std=c89

EXE=noot

DEPS=util.h

OBJ_DEPS=util.o noot.o

%.o: $.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJ_DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f noot
	rm -f *.o

