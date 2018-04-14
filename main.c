#include <stdio.h>
#include "modyfikacje.h"
#define MAXSIZE ( 41943040 )
int 
main (int argc, char* argv[]){
    if(argc==2)
        copy(argv[1],argv[2],MAXSIZE);
    else
        copy(argv[1],argv[2],argv[3]);
  return 0;
}