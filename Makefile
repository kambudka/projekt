CC=gcc
CFLAGS=-I.
DEPS = modyfikacje.h
OBJ = main.o kopiowanie.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)