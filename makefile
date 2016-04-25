CC=gcc
CFLAGS=

EXE=noot

DEPS=

OBJ_DEPS=noot.o

%.o: $.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(EXE): $(OBJ_DEPS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f $(EXE) *.o  *~

