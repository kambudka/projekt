#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ftw.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <syslog.h>
#include "modyfikacje.h"
 
//#define MAXSIZE ( 41943040 )
#define MAXNAME ( 512 ) /* Maksymalna dlugosc sciezki*/
#define BUFFOR ( 1024 ) /* Wielkosc bufora kopiowania */

static int one (const struct dirent *unused){
    return 1;
}

void logg(char* text, char* sciezka)    /*Funkcja logujaca postepy */
{
    char logtext [MAXNAME];
    sprintf(logtext, "%s %s", text, sciezka);   /* Sklejanie informacji 'text' i sciezki do pliku */
	openlog("synchd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1); /* Otwarcie logu */
	syslog(LOG_INFO, logtext);  /* Zapisanie logtext */
	closelog();
}

int kopiuj(char * zrodlo, char * cel)   /* Fukncja kopiujaca pliku */
{
    ssize_t nrd;
    int sfd, dfd ; /* Deskryptory plikow */
    char buffer [BUFFOR]; 
    logg("<info> Kopiowanie Read/WritePliku",zrodlo);
    sfd = open(zrodlo, O_RDONLY);
    dfd = open(cel, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    while (nrd = read(sfd,buffer,sizeof(buffer))){
        write(dfd,buffer,nrd);
    }
    close(sfd);
    close(dfd);
}

void kopiujmmap(char *zrodlo, char *cel)    /* Funkcja mapująca plik do pamięci */
{
    int sfd, dfd;   /* Deskryptory plikow */
    char *source, *dest;    /* Sciezki */
    size_t rozmiar; /* Rozmiar pliku docelowego */

    sfd = open(zrodlo, O_RDONLY);   /* Otworzenie pliku zrodlowego */
    rozmiar = lseek(sfd, 0, SEEK_END);  /* Skopiowanie rozmiaru pliku zrodlewego */
    source = mmap(NULL, rozmiar, PROT_READ, MAP_PRIVATE, sfd, 0);   /* zMMAPowanie pliku zrodlowego */
    dfd = open(cel, O_WRONLY| O_CREAT, 0666);   /* Otworzenie/Utworzenie  pliku docelowego */
    ftruncate(dfd, rozmiar);    /* Rozszerzenie pliku docelowego do wielkosci pliku zrodlowego */

    logg("<info> Kopiowanie MMAP Pliku ","");
    write(dfd, source, rozmiar);    /* Kopiowanie mapy */
    if (dfd != rozmiar) {
        if (dfd == -1)
            logg("<error> Blad kopiowania ","");
    }
    munmap(source, rozmiar);    /* zMUNMAPowanie pamięci */
    close(sfd);
    close(dfd);
}

int modyfikacja(char * zrodlo, char * cel)  /* Funkcja sprawdzajaca daty modyfikacji */
{
    struct stat STATzrodla, STATcelu;
    stat(zrodlo, &STATzrodla);
    stat(cel, &STATcelu);

    time_t t1,t2;           /* Zmienne przechowujace czasy modyfikacji */
    t1 = STATzrodla.st_mtime;
    t2 = STATcelu.st_mtime;

    double diff = difftime(t2,t1);  /* Porownanie czasow modyfikacji */

    if(t1 > t2)
        return 0;
    else if(t1<=t2)
        return 1;
}

void usun_folder(char *sciezka) /* Funkcja usuwające folder i jego zawartosc */
{
    DIR*            dp; /* Struktura DIR przechowujaca pliki */
    struct dirent*  ep; /* Struktura do przechowywania wlasciwosci pliku */
    char            doUsuniecia[512] = {0};

    dp = opendir(sciezka);

    while ((ep = readdir(dp)) != NULL) {    /* Przechodzenie po wszystkich plikach i folderach */
        sprintf(doUsuniecia, "%s/%s", sciezka, ep->d_name);   /* Tworzenie sciezki do usuniecia */
        if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)  /* Jesli to '.' albo '..' idz dalej */
            continue;
        else if (ep->d_type == DT_DIR){ /* Jesli to folder -> rekurencyjne usuwanie */
            logg("<info> Usuwanie Folderu ",ep->d_name);
            usun_folder(doUsuniecia);
        }
        else{
            logg("<info> Usuwanie Pliku ",ep->d_name);
            unlink(doUsuniecia);
        }
    }
    closedir(dp);
    rmdir(sciezka); /* Na koniec usuwanie katalogu */
}

void porownai_usun(char * zrodlo, char * cel)
{
    struct dirent **eps; /* Struktura dirent do przechowywania pliku */
    struct dirent **eps2;   /* Struktura dirent do przechowywania pliku */
    int n,n2;   //Liczba plikow
    int cnt,cnt2;   //Iteratory petli
    char doUsuniecia[MAXNAME] = {0};    /* Sciezka pliku do usuniecia */ 
    n = scandir (zrodlo, &eps, one, alphasort); /* Przeskanowanie folderu zrodlowego */
    n2 = scandir (cel, &eps2, one, alphasort);  /* Przeskanowanie folderu docelowego */
    if (n2 >= 0){
        for(cnt2=0; cnt2<n2; cnt2++){
            int flag = 0;
            for(cnt=0; cnt<n; cnt++){   /* Porownywanie plikow */
                if(strcmp(eps[cnt]->d_name, eps2[cnt2]->d_name)==0){
                    flag = 1;
                    break;
                }
            }
            if(flag==0){    /* Jesli nie znaleziono 2 takich samych plikow */
                sprintf(doUsuniecia, "%s/%s", cel, eps2[cnt2]->d_name); /* Tworzenie sciezki do usuniecia */
            if ((eps2[cnt2]->d_type == DT_REG)){    /* Jesli to zwykly plik -> unlink */
                logg("<info> Usuwanie Pliku ",eps2[cnt2]->d_name);
                unlink(doUsuniecia);
            }
            else if ((eps2[cnt2]->d_type == DT_DIR))    /* Jesli to folder -> rekurencyjne usuwanie */
                usun_folder(doUsuniecia);         
            }
        }
    }
  else
    logg("<error> Nie mozna otworzyc folderu ","");
}

int Synchronizacja(char * zrodlo, char * cel, off_t maxsize, int rflag){

    struct    dirent* plikdir;    /* Struktura dirent do przechowywania pliku */
    struct stat st_buffor;    /* Struktura do przechowywania wlasciwosci pliku*/
    struct stat test;   /* Pomocnicza struktura do porownan */
    DIR* DirWsk = NULL;    /*Wskaznik DIR na otwarty katalog */
    int readfd, writefd;    /* Deskryptory plików */
    char SciezkaDocelowa[MAXNAME] = {0};    /* Sciezka do pliku docelowego*/
    char SciezkaZrodlowa[MAXNAME] = {0};    /* Sciezka do pliku zrodlowego*/

    DirWsk = opendir(zrodlo);    /*Otwarcie katalogu zrodlowego */

    porownai_usun(zrodlo,cel);  /* Porownanie i usuniecie plików nie istniejacych w folderze zrodlowym*/

    if(!DirWsk)
    logg("<error> Nie mozna otworzyc folderu ","");
    else{    
    int nErrNo = 0;
    while (plikdir = readdir(DirWsk)) /* Pojedyncze odczytywanie pliku */
    {
        if(nErrNo == 0 ) 
            nErrNo = errno;
   
        stat(plikdir->d_name, &st_buffor);
        sprintf(SciezkaDocelowa, "%s/%s", cel, plikdir->d_name);
        sprintf(SciezkaZrodlowa, "%s/%s", zrodlo, plikdir->d_name);

        if  (plikdir->d_type == DT_DIR && strcmp(plikdir->d_name, ".") != 0 && strcmp(plikdir->d_name, "..") != 0 && rflag==1){
            mkdir(SciezkaDocelowa, 0777);
            Synchronizacja(SciezkaZrodlowa,SciezkaDocelowa,maxsize,rflag);
        }
        else if ( plikdir->d_type == DT_REG){ 
            modyfikacja(SciezkaZrodlowa,SciezkaDocelowa);
            if(modyfikacja(SciezkaZrodlowa,SciezkaDocelowa)==0){
                stat(SciezkaZrodlowa, &test);
                    if(test.st_size > maxsize)
                        kopiujmmap(SciezkaZrodlowa, SciezkaDocelowa);
                    else
                        kopiuj(SciezkaZrodlowa, SciezkaDocelowa);
            }
            else{
                //logg("Pliki są aktualne\n");
            }

        }
    }
    if(nErrNo != errno){
        //logg("<error> Wystapil blad");
    }
    else
        logg("<info> Synchronizacja zakonczona ","");

    }
    closedir(DirWsk);
    return 0;
}
