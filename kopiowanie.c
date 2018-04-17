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
 
#define MAXSIZE ( 41943040 )
#define MAXNAME ( 64 )
#define BUFFOR ( 1024 )


static int one (const struct dirent *unused){
    return 1;
}

int kopiuj(char * zrodlo, char * cel)
{
    ssize_t nrd;
    int fd;
    int Dpliku1;
    char buffer [BUFFOR];
    //logg("<info> Kopiowanie Read/WritePliku %s",zrodlo);
    fd = open(zrodlo, O_RDONLY);
    Dpliku1 = open(cel, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    while (nrd = read(fd,buffer,sizeof(buffer))){
        write(Dpliku1,buffer,nrd);
    }
    close(fd);
    close(Dpliku1);
}

void logg(char* text){
	openlog("synchd", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	syslog(LOG_INFO, text);
	closelog();
}

void kopiujmmap(char *zrodlo, char *cel)
{
    int sfd, dfd;
    char *source, *dest;

    size_t rozmiar;
    /* SOURCE */
    sfd = open(zrodlo, O_RDONLY);
    rozmiar = lseek(sfd, 0, SEEK_END);
    logg("<info> Kopiowanie MMAP Pliku ");
    source = mmap(NULL, rozmiar, PROT_READ, MAP_PRIVATE, sfd, 0);
    /* DESTINATION */
    dfd = open(cel, O_WRONLY| O_CREAT, 0666);

    ftruncate(dfd, rozmiar);
    /* COPY */
    write(dfd, source, rozmiar);
    if (dfd != rozmiar) {
        if (dfd == -1)
            printf("Error");
    }
    munmap(source, rozmiar);

    close(sfd);
    close(dfd);
}

int modyfikacja(char * zrodlo, char * cel)
{
    struct stat STATzrodla;
    struct stat STATcelu;
    stat(zrodlo, &STATzrodla);
    stat(cel, &STATcelu);
    time_t t1,t2;

    t1 = STATzrodla.st_mtime;
    t2 = STATcelu.st_mtime;

    double diff = difftime(t2,t1);

    if(t1 > t2)
        return 0;
    else if(t1<=t2)
        return 1;
}

void usun_folder(char *sciezka)
{
    DIR*            dp;
    struct dirent*  ep;
    char            p_buf[512] = {0};

    dp = opendir(sciezka);

    while ((ep = readdir(dp)) != NULL) {
        sprintf(p_buf, "%s/%s", sciezka, ep->d_name);
        if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) 
        continue;
        else if (ep->d_type == DT_DIR){
            //sprintf(logtxt, "%s/%s", "<info> Usuwanie Folderu ", ep->d_name);
            //logg(logtxt);
            usun_folder(p_buf);
        }
        else{
            //sprintf(logtxt, "%s/%s", "<info> Usuwanie Pliku ", ep->d_name);
           // logg(logtxt);
            unlink(p_buf);
        }
    }
    closedir(dp);
    rmdir(sciezka);
}

void porownai_usun(char * zrodlo, char * cel)
{
    struct dirent **eps;
    struct dirent **eps2;
    int n,n2;   //Liczba plikow
    int cnt,cnt2;   //Iteratory petli
    char doUsuniecia[MAXNAME] = {0};
    char logtxt[MAXNAME] = {0};
    n = scandir (zrodlo, &eps, one, alphasort);
    n2 = scandir (cel, &eps2, one, alphasort);
    if (n2 >= 0){
        for(cnt2=0; cnt2<n2; cnt2++){
            int flag = 0;
            for(cnt=0; cnt<n; cnt++){
                if(strcmp(eps[cnt]->d_name, eps2[cnt2]->d_name)==0){
                    flag = 1;
                    break;
                }
            }
            if(flag==0){
                sprintf(doUsuniecia, "%s/%s", cel, eps2[cnt2]->d_name);
            if ((eps2[cnt2]->d_type == DT_REG)){
                sprintf(logtxt, "%s/%s", "<info> Usuwanie Pliku ", eps2[cnt2]->d_name);
                logg(logtxt);
                unlink(doUsuniecia);
            }
            else if ((eps2[cnt2]->d_type == DT_DIR))
                usun_folder(doUsuniecia); 
                     
            }
        }
    }
  else
    logg("<error> Nie mozna otworzyc folderu ");
}

int copy(char * zrodlo, char * cel, off_t maxsize, int rflag){

    struct    dirent* spnDirPtr;    /* struct dirent to store all files*/
    struct stat st_buffor;
    struct stat test;
    DIR* pnOpenDir = NULL;    /*DIR Pointer to open Dir*/
    DIR* pnReadDir = NULL;    /*DIR POinter to read directory*/
    int readfd;
    int writefd;
    char strDestFileName[MAXNAME] = {0};
    char strFromFileName[MAXNAME] = {0};
    pnOpenDir = opendir(zrodlo); 

    porownai_usun(zrodlo,cel);
    if(!pnOpenDir)
    logg("<error> Nie mozna otworzyc folderu ");
    else
    {    
    int nErrNo = 0;
    while(spnDirPtr = readdir(pnOpenDir))
    {
        if(nErrNo == 0 ) 
            nErrNo = errno;
   
        stat(spnDirPtr->d_name, &st_buffor);
        sprintf(strDestFileName, "%s/%s", cel, spnDirPtr->d_name);
        sprintf(strFromFileName, "%s/%s", zrodlo, spnDirPtr->d_name);

        if (spnDirPtr->d_type == DT_DIR && strcmp(spnDirPtr->d_name, ".") != 0 && strcmp(spnDirPtr->d_name, "..") != 0 && rflag==1)
        {
            mkdir(strDestFileName, 0777);
            copy(strFromFileName,strDestFileName,maxsize,rflag);
        }
        else if ((spnDirPtr->d_type == DT_REG))
        { 
            modyfikacja(strFromFileName,strDestFileName);
            if(modyfikacja(strFromFileName,strDestFileName)==0){
                  stat(strFromFileName, &test);
                    if(test.st_size > maxsize)
                        kopiujmmap(strFromFileName, strDestFileName);
                    else
                        kopiuj(strFromFileName, strDestFileName);
            }
            else{
                //printf("Pliki są aktualne\n");
            }

    }
    }
    if(nErrNo != errno){
        //logg("<error> Wystapil blad");
    }
    else
        logg("<info> Synchronizacja zakonczona ");

    }
    closedir(pnOpenDir);
    return 0;
}
