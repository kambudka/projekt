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
    int fd1;
    char buffer [BUFFOR];
    fd = open(zrodlo, O_RDONLY);
    fd1 = open(cel, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
    printf("Read/Write\n");
    while (nrd = read(fd,buffer,sizeof(buffer))) {
        write(fd1,buffer,nrd);
    }
    close(fd);
    close(fd1);
}

void kopiujmmap(char *zrodlo, char *cel)
{
    int sfd, dfd;
    char *src, *dest;
    size_t filesize;
    /* SOURCE */
    sfd = open(zrodlo, O_RDONLY);
    filesize = lseek(sfd, 0, SEEK_END);

    src = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, sfd, 0);

    /* DESTINATION */
    dfd = open(cel, O_WRONLY| O_CREAT, 0666);

    ftruncate(dfd, filesize);

    //dest = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, dfd, 0);
    printf("MMAP\n");
    /* COPY */
    write(dfd, src, filesize);
    if (dfd != filesize) {
        if (dfd == -1)
            printf("write");
    }
    munmap(src, filesize);

    close(sfd);
    close(dfd);
}

int modyfikacja(char * zrodlo, char * cel)
{
    struct stat Szrodla;
    struct stat Scelu;
    stat(zrodlo, &Szrodla);
    stat(cel, &Scelu);
    time_t t1,t2;
    t1 = Szrodla.st_mtime;
    t2 = Scelu.st_mtime;
    double diff = difftime(t2,t1);
    //printf("%f\n" ,diff);
    if(t1 > t2)
        return 0;
    //printf(" Zrodlo jest starsze\n");
    else if(t1<=t2)
        return 1;
    //printf("Cel jest zaktualizowany\n");
}

void usun_folder(char *path)
{
    DIR*            dp;
    struct dirent*  ep;
    char            p_buf[512] = {0};

    dp = opendir(path);

    while ((ep = readdir(dp)) != NULL) {
        sprintf(p_buf, "%s/%s", path, ep->d_name);
        if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0) 
        continue;
        else if (ep->d_type == DT_DIR)
            usun_folder(p_buf);
        else
            unlink(p_buf);
    }
    closedir(dp);
    rmdir(path);
}

void porownai_usun(char * pathsrc, char * pathdest)
{
  struct dirent **eps;
  struct dirent **eps2;
  int n;
  int n2;
    int cnt,cnt2;
    char doUsuniecia[MAXNAME] = {0};
  n = scandir (pathsrc, &eps, one, alphasort);
  n2 = scandir (pathdest, &eps2, one, alphasort);
  if (n2 >= 0)
    {
     for(cnt2=0; cnt2<n2; cnt2++)
        {
            int flag = 0;
            for(cnt=0; cnt<n; cnt++)
            {
                if(strcmp(eps[cnt]->d_name, eps2[cnt2]->d_name)==0)
                {
                flag = 1;
                break;
                }
            }
            if(flag==0)
            {
            sprintf(doUsuniecia, "%s/%s", pathdest, eps2[cnt2]->d_name);
            if ((eps2[cnt2]->d_type == DT_REG))
                unlink(doUsuniecia);
            else if ((eps2[cnt2]->d_type == DT_DIR))
                usun_folder(doUsuniecia); 
                     
            }
        }
    }
  else
    perror ("Couldn't open the directory");
    //return eps;
}

int copy(char * arSrcPath, char * arDestPath,off_t maxsize){

    struct    dirent* spnDirPtr;    /* struct dirent to store all files*/
    //struct    dirent* spnDirPtr2;
    struct stat st_buf;
    struct stat test;
    DIR* pnOpenDir = NULL;    /*DIR Pointer to open Dir*/
    DIR* pnReadDir = NULL;    /*DIR POinter to read directory*/
    int readfd;
    int writefd;
    char strDestFileName[MAXNAME] = {0};
    char strFromFileName[MAXNAME] = {0};
    pnOpenDir = opendir(arSrcPath); 

    porownai_usun(arSrcPath,arDestPath);
    if(!pnOpenDir)
    printf("\n ERROR! Directory can not be open");
    else
    {    
    int nErrNo = 0;
    while(spnDirPtr = readdir(pnOpenDir))
    {
        if(nErrNo == 0) nErrNo = errno;
   
        stat(spnDirPtr->d_name, &st_buf);
        sprintf(strDestFileName, "%s/%s", arDestPath, spnDirPtr->d_name);
        sprintf(strFromFileName, "%s/%s", arSrcPath, spnDirPtr->d_name);

        if (spnDirPtr->d_type == DT_DIR && strcmp(spnDirPtr->d_name, ".") != 0 && strcmp(spnDirPtr->d_name, "..") != 0)
        {
            sprintf(strFromFileName, "%s/%s", arSrcPath, spnDirPtr->d_name);
            sprintf(strDestFileName, "%s/%s", arDestPath, spnDirPtr->d_name);
            mkdir(strDestFileName, 0777);
            copy(strFromFileName,strDestFileName,MAXSIZE);
        }
        else if ((spnDirPtr->d_type == DT_REG))
        { 
            modyfikacja(strFromFileName,strDestFileName);
           // int readfd = open(strFromFileName, O_RDONLY);
            //kopiuj(strFromFileName, strDestFileName);
            if(modyfikacja(strFromFileName,strDestFileName)==0){
                  stat(strFromFileName, &test);
                    printf("File size: %d bytes\n", test.st_size);
                    if(test.st_size > MAXSIZE)
                        kopiujmmap(strFromFileName, strDestFileName);
                    else
                        kopiuj(strFromFileName, strDestFileName);
            }
            else{
                printf("Pliki są aktualne\n");
            }

    }
    }
    if(nErrNo != errno)
        printf ("\nERROR Occurred!\n");
    else
        printf ("\nProcess Completed\n");

    }
    closedir(pnOpenDir);
    return 0;
}
