#ifndef modyfikacje_h_
#define modyfikacje_h_
#include <sys/types.h>
static int one (const struct dirent *unused);

int kopiuj(char * zrodlo, char * cel);

void kopiujmmap(char *zrodlo, char *cel);

int modyfikacja(char * zrodlo, char * cel);

void usun_folder(char *path);

void porownai_usun(char * pathsrc, char * pathdest);

int copy(char * arSrcPath, char * arDestPath,off_t maxsize, int rflag);

#endif