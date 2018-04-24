CC=gcc
CFLAGS=-I.
DEPS = modyfikacje.h
OBJ = synchd.o kopiowanie.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

synchd: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)