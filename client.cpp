#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
using namespace std;
/* codul de eroare returnat de anumite apeluri */
extern int errno;

/* portul de conectare la server*/
int port;

int main (int argc, char *argv[])
{
  int sd;			// descriptorul de socket
  struct sockaddr_in server;	// structura folosita pentru conectare 
  		// mesajul trimis
  int nr=0;
  char buf[100]={0};

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }

  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }
  /* Afisare meniul client */

  printf("\n\n\n                                                           COMENZI                                                 ");
  printf("\n 1. login : 'username' 'password' \n 2. register : 'username' 'password' TRUE/FALSE  VOTE_RIGHT(0/1) ");
  /* citirea mesajului */
  char answerBack[1000];
  int WantSExit=0;
  while(strstr(buf,"exit")==NULL)
  {
      printf("\nprompter$:");
      cin.getline(buf,100);
      printf("Comanda data : %s\n",buf);
      //scanf("%d",&nr);
     /* trimiterea mesajului la server */
      if (send (sd,buf,100,0) <= 0)
        {
         perror ("[client]Eroare la write() spre server.\n");
         return errno;
        }
    /* citirea raspunsului dat de server 
     (apel blocant pina cind serverul raspunde) */
     if (recv (sd, answerBack,1000,0) < 0)
      {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
     }
     printf("\n%s",answerBack);
  /* afisam mesajul primit */  
  }
  /* inchidem conexiunea, am terminat */
  close (sd);
}
