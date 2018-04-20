#ifndef modyfikacje_h_
#define modyfikacje_h_
#include <sys/types.h>

static int one (const struct dirent *unused);

int kopiuj(char * zrodlo, char * cel);  /* Fukncja kopiujaca pliku */

void kopiujmmap(char *zrodlo, char *cel);   /* Funkcja mapująca plik do pamięci */

int modyfikacja(char * zrodlo, char * cel);    /* Funkcja sprawdzajaca daty modyfikacji */

void usun_folder(char *path);   /* Funkcja usuwające folder i jego zawartosc */

void porownai_usun(char * pathsrc, char * pathdest);

void logg(char* text, char* sciezka);

int Synchronizacja(char * zrodlo, char * cel,off_t maxsize, int rflag);

#endif