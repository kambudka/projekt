CC=gcc
CFLAGS=-I.
DEPS = modyfikacje.h
OBJ = main1.o kopiowanie.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main1: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)