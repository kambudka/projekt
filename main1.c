#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h> 
#include <syslog.h>
#include <string.h>
#include "modyfikacje.h"

typedef int bool;
#define true 1 
#define false 0
volatile bool breakFlag = true;

bool developer =  true;

int Rflag = 0;
int tflag = 0;
int tvalue = 15;
char svalue[512];
char dvalue[512];
unsigned int wvalue = 1048576;
char *current_dir=NULL;
char buffer[512];

volatile int wakeupFlag = 0;  
int wakeupIterator =0;

void handler(int signum); 

int main (int argc, char **argv)
{
  int index;
  int c;
  opterr = 0; 
  pid_t pid; 

  current_dir = getcwd(buffer, sizeof(buffer));
  printf("Folder: %s\n", current_dir);

  //Przechwytywanie parametrow uruchomienia
  while ((c = getopt (argc, argv, "Rt:s:d:w:")) != -1){
    switch (c)
      {
      case 'R':
        Rflag = 1;
        break;
      case 't':
        tvalue = char_to_int(optarg);
        break;
      case 's':
        get_path(svalue, optarg);
        break;
      case 'd':
        get_path(dvalue, optarg);
        break;
      case 'w':
        wvalue = char_to_int(optarg);
        break;
    
      default:
       ;
      }
  }


  //Przypisanie parametrow uruchomienia gdy sa tylko dwa
  if(argc == 3 && svalue == NULL && dvalue == NULL){
    strcpy(svalue , argv[1]);
    strcpy(dvalue , argv[2]);
    printf("Sorce: %s, destination: %s\n", svalue, dvalue);
    //svalue = argv[1];
    //dvalue = argv[2];
  }else if(svalue == NULL && dvalue == NULL){
    fprintf(stderr, "Brak wymaganych argumentow!\n");
    return 1;
  }

  if(developer)
    printf ("Rflag = %d, tvalue = %d, svalue = %s, dvalue = %s, wvalue=%d \n",
            Rflag, tvalue, svalue, dvalue, wvalue);
  //Powolanie demona
  pid = fork();

  if(pid){
    //Kod dla "rodzica"
    logger("<info> Rozpoczeto synchronizacje.");
  }else{   
    //Czesc kodu dla demona

    //Deklaracja sygnalu synchronizacji
    signal(SIGUSR1, handler);

    logger("<info> Jestem demonem!");

    //Petla glowna demona
    while(1){

      //Gdy flaga obudzenia nie jest podniesiona i czas uspienia uplynal => synch i reset licznika
      if(wakeupFlag != 1 && wakeupIterator == 0){
        logger("<info> Wywolanie synchronizacji po okresolonym czasie");
        //
        //funkcja synchronizacji
        //
        copy(argv[1], argv[2], wvalue);
        wakeupIterator = 15;
        logger("<info> Czasowe uspienie demona");
      }

      //Petka uspienia, break gdy uplynie czas albo bedzie podniesiona flaga
      while(wakeupIterator != 0 && wakeupFlag != 1){
        sleep(2);
        wakeupIterator--;
      }

      //Obsluga podniesionej flagi
      if(wakeupFlag == 1){
        logger("<info> Wywolanie synchronizacji po wykryciu obudzenia");
        //
        //funkcja synchronizacji
        //git
        copy(argv[1], argv[2], wvalue);
        wakeupFlag = 0;
        wakeupIterator = tvalue;
        logger("<info> Czasowe uspienie demona");
      }
            
    }
  }
  return 0;
}

//Przechwycenie sygnalu
void handler(int signum){
  char* napis = "<info> WYKRYTO SYGNAL SIGUSR1!";
	logger(napis);
  wakeupFlag = 1;
}

//Zapisanie do logu
void logger(char* text){
	openlog("synchd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	syslog(LOG_INFO, text);
	closelog();
}

//Zwracanie rozmiaru tablicy char
int my_size_of(char *tablica){
  int ilosc =0;
  while (tablica[ilosc] != '\0'){
    ilosc++;
  }
  return ilosc;
}

//zamiana tablicy char na int
int char_to_int(char* tablica){
  int rozmiar = my_size_of(tablica);
  int dodatkowy_mnoznik=1;
  if(tablica[rozmiar-1] < 48 || tablica[rozmiar-1] > 57){
    switch(tablica[rozmiar-1]){
      case 'B':
        dodatkowy_mnoznik = 1;
        break;
      case 'K':
        dodatkowy_mnoznik = 1024;
        break;
      case 'M':
        dodatkowy_mnoznik = 1024*1024;
        break;
      case 'G':
        dodatkowy_mnoznik = 1024*1024*1024;
        break;
      default:
        ;
    }
    rozmiar--;
  }
  int mnoznik=1;
  for(int i=0; i<rozmiar-1; i++)
    mnoznik *= 10;


  int suma=0;
  for(int i =0; i<rozmiar; i++){
    suma += (tablica[i]-48)*mnoznik;
    if(mnoznik / 10 != 0)
      mnoznik /= 10;
  }
  return suma*dodatkowy_mnoznik;
}

void get_path(char* tablica, char* path)
{
  int dlugosc = my_size_of(path);

  if(path[0] == '/'){
    //strcat(tablica, path);
  }else{
    strcat(tablica, current_dir);
    strcat(tablica, "/");
    strcat(tablica, path);
  }

  if(path[dlugosc-1] != '/'){
    strcat(tablica, "/");
  }
}