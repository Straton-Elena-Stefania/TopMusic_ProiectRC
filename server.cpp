#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>
#include <iostream>
using namespace std;
/* portul folosit */
#define PORT 5459

static int logare=0;  //daca nu m-am conectat
static int IsAdmin=0; //variabila asta daca e 0-nu e admin , iar daca e 1 e admin
static int UserAddWithSucces;
static int HasVoteRight;
char TopMelodii[2000];

sqlite3* db; 

//functia prin care un user adauga un comentariu la o anumita melodie
void comm(sqlite3 *db, char melodie[100],char comm[1000],char User[100],char mesajInapoi[1000])
{
   char *sql=(char *) malloc(1000);
   char *mesajEroare;
   //aceasta operatii reprezinta o inserare in baza de date
   //exemplu: INSERT INTO Comentarii VALUES('Bogdan','Arunca-ma','Delia esti geniala!');
   strcpy(sql,"INSERT INTO Comentarii VALUES(");
   strcat(sql,User);
   strcat(sql,",");
   strcat(sql,melodie);
   strcat(sql,",");
   strcat(sql,comm);
   strcat(sql,");");
   cout << "adaugare: " << sql << endl;
   int rc ;
   rc = sqlite3_open("StefaniaDataBase.db",&db);//pentru a realiza o operatie cu baza de date, mai intai trebuie sa o deschidem
   rc = sqlite3_exec(db,sql,NULL,0,&mesajEroare);
   if(rc !=SQLITE_OK)
   {
     strcpy(mesajInapoi,"\n\n Ne pare rau , a aparut o problema ! Incercati sa scrieti toate datele corect! \n\n");
     fprintf(stderr,"SQL error: %s\n",mesajEroare);
     sqlite3_free(mesajEroare);
  }else {
         strcpy(mesajInapoi, "\n\nComentariul a fost adaugat! \n\n");
        }
  sqlite3_close(db);// inchidem baza de date
}

//functia prin care un membru al aplicatiei poate vota
void votareMelodie(sqlite3 *db, char melodie[100],char answerBack[1000])
{
   //aceasta operatie reprezinta un update in baza de date, mai exact trebuie 
   //sa incrementam din tabela Melodii ,coloana cu numar_voturi, linia in care sa afla melodia respectiva si sa o incrementam
   //exemplu: UPDATE Melodii SET numar_voturi=numar_voturi+1 WHERE trim(nume)='Arunca-ma';
  char *sql=(char*)malloc(1000);
  char *mesajEroare;
  strcpy(sql,"UPDATE Melodii SET numar_voturi=numar_voturi+1 WHERE trim(nume)=");
  strcat(sql,melodie);
  strcat(sql,";");
  cout << sql << endl;

  int rc ;
  rc = sqlite3_open("StefaniaDataBase.db",&db);
  rc = sqlite3_exec(db,sql,NULL,0,&mesajEroare);
  if(rc !=SQLITE_OK)
  {
    strcpy(answerBack,"\n\n Ne pare rau , a aparut o problema ! \n\n");
    fprintf(stderr,"SQL error: %s\n",mesajEroare);
    sqlite3_free(mesajEroare);
  }else {
          strcpy(answerBack, "Votarea melodiei s-a realizat cu succes! \n");
         }
  sqlite3_close(db);
}

static int CheckIfExistsUser (void *str, int argc, char **argv, char **azColName)//functia de callback
{
    int x;
    for (x = 0; x < argc; x++)
    {
        if(argv[x]?argv[x]:"NULL")
        {
          logare=1;
        }
    }
    return 0;
}
int CheckIfIsAdmin (void *str, int argc, char **argv, char **azColName)
{
    int x;
    for (x = 0; x < argc; x++)
    {
        if(argv[x]?argv[x]:"NULL")
        {
          IsAdmin=1;
        }
    }
    return 0;
}
int CheckIfRightVote (void *str, int argc, char **argv, char **azColName)
{
    int x;
    for (x = 0; x < argc; x++)
    {
        if(argv[x]?argv[x]:"NULL")
        {
          HasVoteRight=1;
        }
    }
    return 0;
}
static int CheckIfTop (void *str, int argc, char **argv, char **azColName)//functia de callback
{

    strcat(TopMelodii,argv[0]);
    strcat(TopMelodii," ");
    strcat(TopMelodii,argv[1]);
    strcat(TopMelodii," ");
    strcat(TopMelodii,"\n");
   // printf("%s",argv[0]);
    return 0;
}

//aceasta functie este folosita pentru functionalitatea register
//cand se inregistreaza un anumit utilizator acesta trebuie introdus in baza de date print-un insert
void AddUser(sqlite3 *db ,char UserNew[100],char PassNew[100],char IsAdmin[100],char voteRight[100])
{
  //ex: INSERT INTO Users VALUES('Ionel','123','FALSE','TRUE')
  char * sql = (char*)malloc(1000);
  char *mesajEroare;
  strcpy(sql,"INSERT INTO Users VALUES(");
  strcat(sql,UserNew);
  strcat(sql,",");
  strcat(sql,PassNew);
  strcat(sql,",");
  strcat(sql,IsAdmin);//daca este admin
  strcat(sql,",");
  strcat(sql,voteRight);//daca are drept de vot
  strcat(sql,");");

  int rc;
  rc = sqlite3_open("StefaniaDataBase.db",&db);
  rc = sqlite3_exec(db,sql,NULL,0,&mesajEroare);
  if(rc !=SQLITE_OK)
  {
    fprintf(stderr,"SQL error: %s\n",mesajEroare);
    sqlite3_free(mesajEroare);
  }else{
         UserAddWithSucces=1;
        }
  sqlite3_close(db);
}

//http://zetcode.com/db/sqlitec/
//https://www.geeksforgeeks.org/sql-using-c-c-and-sqlite/?fbclid=IwAR3r3m-4JgvY9PBYd8sxNyC3vToW-zBAqFWYNORZM6LWaeBkIEWdyQMGm8Y

//functia de autentificare 
void login(sqlite3 *db,char user[100],char pass[100])
{
    char *sql=(char*)malloc(1000);
    char *zErrMsg=0;
    //formam interogarea prin care cautam in baza de date existenta username-ului cu parola corespondenta
    strcpy(sql,"SELECT * FROM Users WHERE username=");
    strcat(sql,user);
    strcat(sql," AND password=");
    strcat(sql,pass);
    strcat(sql,";");

    int rc = sqlite3_exec (db, sql, CheckIfExistsUser, 0, &zErrMsg);
  
    if (rc != SQLITE_OK)
       {
        fprintf (stderr,"SQL error: %s\n", zErrMsg);
        sqlite3_free (zErrMsg);
       }
   sqlite3_close(db);
}

//functia care recunoaste daca utilizatorul este un admin 
void IsAdminThisUser(sqlite3 *db,char nume[100],char parola[100])
{
    char *sql=(char*)malloc(1000);
    char *zErrMsg=0;//mesajul de eroare
    //formam interogarea prin care cautam in baza de date existenta username-ului cu parola corespondenta
    strcpy(sql,"SELECT * FROM Users WHERE username=");
    strcat(sql,nume);
    strcat(sql," AND password=");
    strcat(sql,parola);
    strcat(sql," AND Admin=1 ");
    strcat(sql,";");
   
    int rc ;
    rc = sqlite3_open("StefaniaDataBase.db",&db);
    rc=sqlite3_exec (db, sql, CheckIfIsAdmin, 0, &zErrMsg);
  
    if (rc != SQLITE_OK)
        {
            fprintf (stderr,"SQL error: %s\n", zErrMsg);
            sqlite3_free (zErrMsg);
        }
   sqlite3_close(db);
}

//functia care afiseaza topul general in functie de numarul de voturi
//aceasta functie reda o fraza select din tabela Melodii ce aseaza in ordine descrescatoare dupa nr de voturi toate melodiile
void TopMusic(sqlite3 *db,char mesajInapoi[1000])
{  
    bzero(TopMelodii,2000);
    char *sql=(char*)malloc(1000);
    char *zErrMsg=0;
    strcpy(sql,"SELECT nume,numar_voturi FROM Melodii ORDER BY numar_voturi DESC;");

   int rc ;
   rc = sqlite3_open("StefaniaDataBase.db",&db);
   rc = sqlite3_exec(db,sql,CheckIfTop,0,&zErrMsg);
   if(rc !=SQLITE_OK)
     {
         strcpy(mesajInapoi,"\n\n Ne pare rau , a aparut o problema ! \n\n");
         fprintf(stderr,"SQL error: %s\n",zErrMsg);
         sqlite3_free(zErrMsg);
     }else{
            strcpy(mesajInapoi,TopMelodii);
           }
  sqlite3_close(db);

}

//functia care afiseaza topul dupa gen in functie de numarul de voturi
//aceasta functie reda o fraza select din tabela Melodii, care selecteaza dupa gen numele melodiei si nr de voturi, ce aseaza in ordine descrescatoare dupa nr de voturi toate melodiile
void TopMusicGen(sqlite3 *db,char mesajInapoi[1000],char gen[100])
{
    bzero(TopMelodii,2000);
    char *sql=(char*)malloc(1000);
    char *zErrMsg=0;
    //exemplu: Select nume, numar_voturi from Melodii where gen='rock' order by numar_voturi desc;
    strcpy(sql,"SELECT nume, numar_voturi from Melodii where gen=");
    strcat(sql,gen);
    strcat(sql,"  order by numar_voturi desc;");
   

   int rc ;
   rc = sqlite3_open("StefaniaDataBase.db",&db);
   rc = sqlite3_exec(db,sql,CheckIfTop,0,&zErrMsg);
   if(rc !=SQLITE_OK)
   {
     strcpy(mesajInapoi,"\n\n Ne pare rau , a aparut o problema ! \n\n");
     fprintf(stderr,"SQL error: %s\n",zErrMsg);
     sqlite3_free(zErrMsg);
  }else{
         strcpy(mesajInapoi,TopMelodii);
        }
  sqlite3_close(db);
    
}

//doar admini
//stergerea unei melofii din top
//reprezinta o fraza delete, ce sterge linia corespunzatoare numele melodiei
void stergereMelodii(sqlite3 *db,char nume[100],char MesajDupa[30])
{
    char *sql=(char*)malloc(1000);
    char *zErrMsg=0;
    strcpy(sql,"delete from Melodii where trim(nume)=");
    strcat(sql,nume);
    strcat(sql," ;");
    int rc=sqlite3_open("StefaniaDataBase.db",&db);
    rc= sqlite3_exec (db, sql,NULL, 0, &zErrMsg);
    if (rc != SQLITE_OK)
    {
     fprintf (stderr,"SQL error: %s\n", zErrMsg);
     sqlite3_free (zErrMsg);
     strcpy(MesajDupa,"Nu aveti acces la aceasta functionalitate!");
    }else
        {strcpy(MesajDupa,"Melodia a fost eliminata din lista cu succes!");}
   sqlite3_close(db);
}

//doar pt admini
//functia care adauga o melodie in top
//aceasta reprezintao inserare in baza de date 
void adaugareTop(sqlite3 *db,char nume[100],char descriere[100],char gen[100], char link[100],char MesajDupa[30])
{
  //ex insert into Melodii values ('Highway to Hell ','Este o melodie a trupei rock australiene AC / DC.Este piesa de deschidere a albumului lor din 1979 Highway to Hell.','rock','https://www.youtube.com/watch?v=gEPmA3USJdI%7C9',0);
  char * sql = (char*)malloc(1000);
  char *mesajEroare;
  strcpy(sql,"INSERT INTO Melodii VALUES(");
  strcat(sql,nume);
  strcat(sql,",");
  strcat(sql,descriere);
  strcat(sql,",");
  strcat(sql,gen);
  strcat(sql,",");
  strcat(sql,link);
  strcat(sql,", 0 ");
  strcat(sql,");");

  int rc;
  rc = sqlite3_open("StefaniaDataBase.db",&db);
  rc = sqlite3_exec(db,sql,NULL,0,&mesajEroare);
  if(rc !=SQLITE_OK)
  {
    fprintf(stderr,"SQL error: %s\n",mesajEroare);
    sqlite3_free(mesajEroare);
    strcpy(MesajDupa,"Ceva s-a intamplat... date incomplete");
  }else{
    strcpy(MesajDupa,"Adaugare cu succes");;
  }
  sqlite3_close(db);
}

//functia are va ajuta functionalitatea legata de restrictionarea votului
//aceasta determina daca utilizatorul are sau nu dreptul de a vota;
void HasTheRightToVote(sqlite3 *db,char nume[100])
{
    char *sql=(char*)malloc(1000);
    char *zErrMsg=0;
    strcpy(sql,"SELECT * FROM Users WHERE username=");
    strcat(sql,nume);
    strcat(sql," AND Vote=1");
    strcat(sql,";");
    //afisam introgarea
    //printf("INTEROGAREA : %s",sql);
    int rc=sqlite3_open("StefaniaDataBase.db",&db);
    rc= sqlite3_exec (db, sql,CheckIfRightVote, 0, &zErrMsg);
    if (rc == SQLITE_OK)
    {
     fprintf (stderr,"SQL error: %s\n", zErrMsg);
     sqlite3_free (zErrMsg);
    }
   sqlite3_close(db);
}

//restrictionarea votului
//aceasta functionalitate decrementeaza din tabela Users coloana vote si linia corespunzatoare numelui
void DeleteRight(sqlite3 *db ,char username[100],char answBack[1000])
{
  char *sql=(char *)malloc(1000);
  char *eroare;
  strcpy(sql,"UPDATE Users SET Vote=0");
  strcat(sql," WHERE username=");
  strcat(sql,username);
  strcat(sql,";");
  //cout << "Interogare: " << sql << endl;
  int rc = sqlite3_open("StefaniaDataBase.db",&db);
  rc = sqlite3_exec(db,sql,NULL,0,&eroare);
  if(rc !=SQLITE_OK)
  { strcpy(answBack,"\n Nu poti revoca dreptul de a vota!\n");
     fprintf (stderr,"SQL error: %s\n", eroare);
     sqlite3_free (eroare);
  }else{
        strcpy(answBack,"\n Dreptul de a vota a fost revocat cu succes!\n");
        }
  sqlite3_close(db);
}

//raspunsul
void AnswerClient(int client_socket) 
{
  char comandaDeLaClient[100]={0};
  char RaspunsCatreClient[1000]={0};
  char NumeUtilizator[100];
  char Parola[100];
  int IsLogged=0;//daca sunt logat
  int ExitWants=0;
  UserAddWithSucces=0;
  HasVoteRight=0;
  char UserToAdd[100]={0},PassToAdd[100]={0},newAdmin[100]={0},VoteRight[20];
  while(ExitWants==0)
  {
     bzero(RaspunsCatreClient,1000);
     recv(client_socket,comandaDeLaClient,100,0);
     cout << "Comanda primita de la client : " << comandaDeLaClient << endl;
     if(strstr(comandaDeLaClient,"exit")!=NULL)
     {
       ExitWants=1;
       strcpy(RaspunsCatreClient,"\n\n Va-ti deconectat! Va mai asteptam!                \n\n");
     }
     if(strstr(comandaDeLaClient,"login")!=NULL)
     {
        bzero(NumeUtilizator,100);
        bzero(Parola,100);
        // l o g i n _ : _ '     '  _ '   '
        // 0 1 2 3 4 5 6 7 8 
        int index_first_space=8,index_User_Pass=0;
        while(comandaDeLaClient[index_first_space]!=' ')
        {
           NumeUtilizator[index_User_Pass]=comandaDeLaClient[index_first_space];
           index_first_space++;
           index_User_Pass++;
        }
        index_first_space++;
        index_User_Pass=0;
        while(comandaDeLaClient[index_first_space]!=' ')
        {
          Parola[index_User_Pass]=comandaDeLaClient[index_first_space];
          index_first_space++;
          index_User_Pass++;
        }

        login(db,NumeUtilizator,Parola);//apelam functia de login

        if(logare == 1)
        {
          IsLogged=1;
          IsAdminThisUser(db,NumeUtilizator,Parola);//apelam functia pt admin
          if(IsAdmin == 1)
          {
             strcpy(RaspunsCatreClient,"\n\n Bine ati venit (MENU ADMIN ) \n\n1.vot ’nume melodie’ \n2.Top\n3.TopGen 'gen muzical'\n4.comm 'nume melodie' 'comm'\n5.adaugareTop ’numele melodiei’ ’descriere’ ’gen’ ’link'""\n6.Sterge ’nume melodie'\n7.deleteRight ’nume utilizator’\n8.exit\n");
          }else{
              strcpy(RaspunsCatreClient,"\n\n BINET ATI VENIT (MENU USER NORMAL )! \n\n1.vot ’nume melodie’ \n2.Top\n3.TopGen 'gen muzical'\n4.comm 'nume melodie' 'comm'\n5.adaugareTop ’numele melodiei’ ’descriere’ ’gen’ ’link'\n6.exit\n");
          }
        }else{
          strcpy(RaspunsCatreClient,"\n Ne pare rau \n");
        }
        logare=0;  
     }
     if(strstr(comandaDeLaClient,"register")!=NULL)
     {
       //  register   :     '  nume' 'pass' 'true' 'true'
       // 01234567 8 9 10 11
        int first_index_space=11,Index_NewUser=0;
        while(comandaDeLaClient[first_index_space]!=' ')
        {
          UserToAdd[Index_NewUser]=comandaDeLaClient[first_index_space];
          first_index_space++;
          Index_NewUser++;
        }
        first_index_space++;
        Index_NewUser=0;
        while(comandaDeLaClient[first_index_space]!=' ')
        {
          PassToAdd[Index_NewUser]=comandaDeLaClient[first_index_space];
          Index_NewUser++;
          first_index_space++;
        }
        Index_NewUser=0;
        first_index_space++;
        while(comandaDeLaClient[first_index_space]!=' ')
        {
          newAdmin[Index_NewUser]=comandaDeLaClient[first_index_space];
          first_index_space++;
          Index_NewUser++;
        }
        if(strstr(newAdmin,"TRUE")!=NULL)
        {
          strcpy(newAdmin,"1");
        }else{
          strcpy(newAdmin,"0");
        }
        Index_NewUser=0;
        first_index_space++;
        while(comandaDeLaClient[first_index_space]!=' ')
        {
          VoteRight[Index_NewUser]=comandaDeLaClient[first_index_space];
          Index_NewUser++;
          first_index_space++;
        }
        if(strstr(VoteRight,"TRUE")!=NULL)
        {
          strcpy(VoteRight,"1");
        }else{
          strcpy(VoteRight,"0");
        }
        AddUser(db,UserToAdd,PassToAdd,newAdmin,VoteRight);//apelam functia de register
        if(UserAddWithSucces==1)
        {
          strcpy(RaspunsCatreClient,"\n\n  Utilizator inregistrat cu succes ! \n\n");
        }else{
          strcpy(RaspunsCatreClient,"\n\n  Ne pare rau , ati gresit undeva !  \n\n");
        }
     }
     if((strstr(comandaDeLaClient,"deleteRight")!=NULL)&&(IsAdmin==1))//restrctionarea votului, care se face doar de administrator
     {
        char UserDelete[100]={0};
        int Index_Delete=12,indexUser=0;
        while(comandaDeLaClient[Index_Delete]!=' ')
        {
          UserDelete[indexUser]=comandaDeLaClient[Index_Delete];
          Index_Delete++;
          indexUser++;
        }
        DeleteRight(db,UserDelete,RaspunsCatreClient);//apelam functia de restrictionare
     }
     if((strstr(comandaDeLaClient,"vot")!=NULL)&&(IsLogged==1))//functia de vot
     {
        char melodie[100]={0};
        int indexSpace=4,indexMelodie=0;
        HasTheRightToVote(db,NumeUtilizator);
        if(HasVoteRight==1)
        {
          while(comandaDeLaClient[indexSpace]!=' '){ 
              melodie[indexMelodie]=comandaDeLaClient[indexSpace];
              indexSpace++;
              indexMelodie++;
           }
          //cout << "Melodie=" << "+" << melodie << "+" << endl;
          strcpy(RaspunsCatreClient,melodie);
          votareMelodie(db,melodie,RaspunsCatreClient);
        }else{
          strcpy(RaspunsCatreClient,"\n\n Ne pare rau nu ai drept de vot ! \n\n");
        }
     }
     if((strstr(comandaDeLaClient,"comm")!=NULL)&&(IsLogged==1))//comentariu
     {
       char melodie[100]={0},comentariu[300]="\'";
       int primul_spatiu=5,index_Melodie_comentariu=0;
          while(comandaDeLaClient[primul_spatiu]!=' ')
          {
            melodie[index_Melodie_comentariu]=comandaDeLaClient[primul_spatiu];
            primul_spatiu++;
            index_Melodie_comentariu++;
          }
          index_Melodie_comentariu=1;
          primul_spatiu=primul_spatiu+2;
          while(comandaDeLaClient[primul_spatiu]!='\''){
              comentariu[index_Melodie_comentariu]=comandaDeLaClient[primul_spatiu];
              index_Melodie_comentariu++;
              primul_spatiu++;
           }
          strcat(comentariu,"\'");
          comm(db,melodie,comentariu,NumeUtilizator,RaspunsCatreClient);
     }
      if((strstr(comandaDeLaClient,"Top")!=NULL)&&(IsLogged==1))//afisarea topului general
      {
       TopMusic(db,RaspunsCatreClient);
      }
    if((strstr(comandaDeLaClient,"TopGen")!=NULL)&&(IsLogged==1))//afisarea topului dupa gen
    { 
     char gen[30]={0};
     int primul_spatiu=7,index_gen=0;
     for(int i=7;comandaDeLaClient[i]!=' ';i++)
        {
          gen[index_gen]=comandaDeLaClient[i];
          index_gen++;
         }
     printf("+%s+",gen);
     TopMusicGen(db,RaspunsCatreClient,gen);
     }
       if((strstr(comandaDeLaClient,"Sterge")!=NULL) && (IsAdmin==1))//stergerea unei melodii din top
      { 
      char nume[30]="\'";
      int index_nume=1;
      for(int i=8;comandaDeLaClient[i]!='\'';i++)
           {
           nume[index_nume]=comandaDeLaClient[i];
           index_nume++;
             }
       strcat(nume,"\'" );
       printf("+%s+",nume);
      stergereMelodii(db,nume,RaspunsCatreClient);
     }
  if((strstr(comandaDeLaClient,"adaugareTop")!=NULL)&&(IsLogged==1))//adaugarea unei melodii din top
     {
       //  a  d  a  u g a r e T o p   _  '   A  n  a  M  a  r  i  a  '  _  '
       //  0  1  2  3 4 5 6 7 8 9 10 11  12  13 14 15 16 17 18 19 20 21 22 23
       char nume2[100]="\'",descriere[100]="\'",gen[100]="\'", link[100]="\'";
       int index=1,i;
       for( i=13;comandaDeLaClient[i]!='\'';i++)
            {   
                nume2[index]=comandaDeLaClient[i];
                index++;
             }
        strcat(nume2,"\'");
        //strcpy(RaspunsCatreClient,nume2);
        i=i+3;
        index=1;
        while(comandaDeLaClient[i]!='\'')
        {
          descriere[index]=comandaDeLaClient[i];
          index++;
          i++;
        }
        strcat(descriere,"\'");
        //strcpy(RaspunsCatreClient,descriere);
        i=i+3;
        index=1;
        while(comandaDeLaClient[i]!='\'')
        {
          gen[index]=comandaDeLaClient[i];
          index++;
          i++;
        }
        strcat(gen,"\'");
        //index_descriere=index_nume2+1;
         //strcpy(RaspunsCatreClient,gen); 
         i=i+3;
        index=1;
        while(comandaDeLaClient[i]!='\'')
        {
          link[index]=comandaDeLaClient[i];
          index++;
          i++;
        }
        strcat(link,"\'");
        //strcpy(RaspunsCatreClient,link);
        adaugareTop(db,nume2,descriere,gen, link,RaspunsCatreClient);
     }

     send(client_socket,RaspunsCatreClient,1000,0);
  }
    
  
  close(client_socket);//inchidem comunicarea cu clientul
}
 
int main()
{
  int socketS;
  struct sockaddr_in server, client; // structura folosita de server si client
  int client_socket;
  socklen_t lungime;
  
  /* crearea unui socket */
  socketS = socket(AF_INET, SOCK_STREAM, 0);
  if (socket < 0) 
    {
      printf("Eroare la crearea socketului server\n");
      return 1;
    }

    int rc = sqlite3_open("StefaniaDataBase.db", &db);//stabilire conexiune baza date
    if (rc)	 //verificare conexiune
    {
        fprintf(stderr, "error: %s\n", sqlite3_errmsg (db));
        return 0;
    }

  memset(&server, 0, sizeof(server));
   /* utilizam un port utilizator */
  server.sin_port = htons(5459);
  /* umplem structura folosita de server */
  /* stabilirea familiei de socket-uri */
  server.sin_family = AF_INET;
  /* acceptam orice adresa */
  server.sin_addr.s_addr = INADDR_ANY;
  
  /* atasam socketul */
  if (bind(socketS, (struct sockaddr *) &server, sizeof(server)) < 0)
     {
        printf("Eroare la bind\n");
        return 1;
      }
 
  listen(socketS, 5); /* punem serverul sa asculte daca vin clienti sa se conecteze */
  
  lungime = sizeof(client);
  memset(&client, 0, sizeof(client));
  
  while (1) 
    {
    client_socket = accept(socketS, (struct sockaddr *) &client, &lungime);
    printf("S-a conectat un client.\n");
    if (fork() == 0) //se creaza un proces copil
        {
         logare=0;
         IsAdmin=0;
         AnswerClient(client_socket);
         exit(0);
        }  
   }
}
