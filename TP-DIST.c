/* T. Grandpierre - Application distribue'e pour TP IF4-DIST 2004-2005

But : 

fournir un squelette d'application capable de recevoir des messages en 
mode non bloquant provenant de sites connus. L'objectif est de fournir
une base pour implementer les horloges logique/vectorielle/scalaire, ou
bien pour implementer l'algorithme d'exclusion mutuelle distribue'

Syntaxe :
         arg 1 : Numero du 1er port
	 arg 2 et suivant : nom de chaque machine

--------------------------------
Exemple pour 3 site :

Dans 3 shells lances sur 3 machines executer la meme application:

pc5201a>./dist 5000 pc5201a.esiee.fr pc5201b.esiee.fr pc5201c.esiee.fr
pc5201b>./dist 5000 pc5201a.esiee.fr pc5201b.esiee.fr pc5201c.esiee.fr
pc5201c>./dist 5000 pc5201a.esiee.fr pc5201b.esiee.fr pc5201c.esiee.fr

pc5201a commence par attendre que les autres applications (sur autres
sites) soient lance's

Chaque autre site (pc5201b, pc5201c) attend que le 1er site de la
liste (pc5201a) envoi un message indiquant que tous les sites sont lance's


Chaque Site passe ensuite en attente de connexion non bloquante (connect)
sur son port d'ecoute (respectivement 5000, 5001, 5002).
On fournit ensuite un exemple permettant 
1) d'accepter la connexion 
2) lire le message envoye' sur cette socket
3) il est alors possible de renvoyer un message a l'envoyeur ou autre si
necessaire 

*/


#include <stdio.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<fcntl.h>
#include<netdb.h>
#include<string.h>
#include<stdbool.h>
#include<time.h>

#define max(a,b) (((a)>(b))?(a):(b))

struct clock {
	int time;
	int si;
};

int GetSitePos(int Nbsites, char *argv[]) ;
void WaitSync(int socket);
void SendSync(char *site, int Port);
void SendClock(int dest, struct clock *c);

/*Identification de ma position dans la liste */
int GetSitePos(int NbSites, char *argv[]) {
  char MySiteName[20]; 
  int MySitePos=-1;
  int i;
  gethostname(MySiteName, 20);
  for (i=0;i<NbSites;i++) 
    if (strcmp(MySiteName,argv[i+2])==0) {
      MySitePos=i;
      //printf("L'indice de %s est %d\n",MySiteName,MySitePos);
      return MySitePos;
    }
  if (MySitePos == -1) {
    printf("Indice du Site courant non trouve' dans la liste\n");
    exit(-1);
  }
  return (-1);
}


/*Attente bloquante d'un msg de synchro sur la socket donne'e*/
void WaitSync(int s_ecoute) {
  char texte[40];
  int l;
  int s_service;
  struct sockaddr_in sock_add_dist;
  unsigned int size_sock;
  size_sock=sizeof(struct sockaddr_in);
  printf("WaitSync : ");fflush(0);
  s_service=accept(s_ecoute,(struct sockaddr*) &sock_add_dist,&size_sock);
  l=read(s_service,texte,39);
  texte[l] ='\0';
  printf("%s\n",texte); fflush(0);
  close (s_service);
} 

/*Envoie d'un msg de synchro a la machine Site/Port*/
void SendSync(char *Site, int Port) {
  struct hostent* hp;
  int s_emis;
  char chaine[15];
  int longtxt;
  struct sockaddr_in sock_add_emis;
  int size_sock;
  
  if ( (s_emis=socket(AF_INET, SOCK_STREAM,0))==-1) {
    perror("SendSync : Creation socket");
    exit(-1);
  }
    
  hp = gethostbyname(Site);
  if (hp == NULL) {
    perror("SendSync: Gethostbyname");
    exit(-1);
  }

  size_sock=sizeof(struct sockaddr_in);
  sock_add_emis.sin_family = AF_INET;
  sock_add_emis.sin_port = htons(Port);
  memcpy(&sock_add_emis.sin_addr.s_addr, hp->h_addr, hp->h_length);
  
  if (connect(s_emis, (struct sockaddr*) &sock_add_emis,size_sock)==-1) {
    perror("SendSync : Connect");
    exit(-1);
  }
     
  sprintf(chaine,"**SYNCHRO**");
  longtxt =strlen(chaine);
  /*Emission d'un message de synchro*/
  write(s_emis,chaine,longtxt);
  close (s_emis); 
}

int compare(const void* a, const void* b)
{
	struct clock ca = * ( (struct clock*) a );
	struct clock cb = * ( (struct clock*) b );
	
	int int_a = ca.time;
	int int_b = cb.time;

	if ( int_a == int_b ) return 0;
	else if ( int_a < int_b ) return -1;
	else return 1;
}

void resetAccord(bool* tab, int taille)
{
	for(int i=0; i<taille; i++)
	{
		tab[i]=0;
	}
}

bool testAccord(bool* tab, int taille)
{
	int res=1;
	for(int i=0; i<taille; i++)
	{
		if(tab[i]==0)
		{
			return 0;
		}
	}
	return res;
}

void SendMsg(int dest, char *buf)
{
	struct sockaddr_in sock_add;
	int client;
	int size_sock_add, longtxt;
	struct hostent* hp;

	if ( (client=socket(AF_INET, SOCK_STREAM,0))==-1) {
		perror("Creation socket");
		exit(-1);
	}
	
	hp = gethostbyname("localhost");
	if (hp == NULL) {
		perror("client");
		exit(-1);
  }

	size_sock_add=sizeof(struct sockaddr_in);
	sock_add.sin_family = AF_INET;
	sock_add.sin_port = htons(dest);
	memcpy(&sock_add.sin_addr.s_addr, hp->h_addr, hp->h_length);
	
	if (connect(client, (struct sockaddr*) &sock_add, size_sock_add )==-1) {
		perror("Probleme connect"); }
	else {
        longtxt =strlen(buf);
		write(client,buf,longtxt+1);
		close (client);
	}
}

/***********************************************************************/
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

int main (int argc, char* argv[]) {
  struct sockaddr_in sock_add, sock_add_dist;
  unsigned int size_sock;
  int s_ecoute, s_service;
  char texte[40];
  char chaine[40];
  char chaine2[40];
  int i, l, size;
  struct clock *p, *d, *end;
  float t;
  int rd, nb_file;
  bool demande;
  struct clock hl, new_clock;
  struct clock file[100];
  char *extract;
  
  demande=0;
  nb_file=0;
  int PortBase=-1; /*Numero du port de la socket a` creer*/
  int NSites=-1; /*Nb total de sites*/
  int site0=-1;

  if (argc!=4) {
    printf("Il faut donner le numero de port du site et le nombre total de sites");
    exit(-1);
  }

  /*----Nombre de sites (adresses de machines)---- */
  //NSites=argc-2;
  NSites=atoi(argv[3]); //-2;

  /*CREATION&BINDING DE LA SOCKET DE CE SITE*/
  PortBase=atoi(argv[2]) ;//+GetSitePos(NSites, argv);
  printf("Numero de port de ce site %d\n",PortBase);
  
  site0=atoi(argv[1]);
  
  srand(time(0)%PortBase);
  
  hl.si=PortBase;
  hl.time=0;
  bool accord[NSites];
  resetAccord(accord, NSites);

  sock_add.sin_family = AF_INET;
  sock_add.sin_addr.s_addr= htons(INADDR_ANY);  
  sock_add.sin_port = htons(PortBase);

  if ( (s_ecoute=socket(AF_INET, SOCK_STREAM,0))==-1) {
    perror("Creation socket");
    exit(-1);
  }

  if ( bind(s_ecoute,(struct sockaddr*) &sock_add, \
	    sizeof(struct sockaddr_in))==-1) {
    perror("Bind socket");
    exit(-1);
  }
  
  listen(s_ecoute,30);
  /*----La socket est maintenant cre'e'e, binde'e et listen----*/

  if (strcmp(argv[1], argv[2])==0) { 
    /*Le site 0 attend une connexion de chaque site : */
    for(i=0;i<NSites-1;i++) 
      WaitSync(s_ecoute);
    printf("Site 0 : toutes les synchros des autres sites recues \n");fflush(0);
    /*et envoie un msg a chaque autre site pour les synchroniser */
    for(i=0;i<NSites-1;i++) 
      SendSync("localhost", atoi(argv[1])+i+1);
    } else {
      /* Chaque autre site envoie un message au site0 
	 (1er  dans la liste) pour dire qu'il est lance'*/
      SendSync("localhost", atoi(argv[1]));
      /*et attend un message du Site 0 envoye' quand tous seront lance's*/
      printf("Wait Synchro du Site 0\n");fflush(0);
      WaitSync(s_ecoute);
      printf("Synchro recue de  Site 0\n");fflush(0);
  }
  
  /* Passage en mode non bloquant du accept pour tous*/
  /*---------------------------------------*/
  fcntl(s_ecoute,F_SETFL,O_NONBLOCK);
  size_sock=sizeof(struct sockaddr_in);
  
  /* Boucle infini*/
  while(1) {
  
    /* On commence par tester l'arrivee d'un message */
    s_service=accept(s_ecoute,(struct sockaddr*) &sock_add_dist,&size_sock);
    if (s_service>0) {
		/*Extraction et identification du message */
		l=read(s_service,texte,39);
		texte[l] ='\0';
		printf("Message recu : %s",texte); fflush(0);
		close (s_service);
		
		//on extrait l'estampille temporelle des messages
		extract=texte+1;
		new_clock.time=atoi(strsep(&extract, ","));
		new_clock.si=atoi(strsep(&extract, ")"));
		sprintf(chaine2, "%s",extract+1); //+1 pour l'espace après le ")"
		
		hl.time=max(hl.time,new_clock.time)+1;
		printf(" : evenement %i ",hl.time);
		
		if(strncmp("REQ", chaine2, 3)==0)
		{
			//le message est une requete de SC			
			file[nb_file]=new_clock;
			nb_file++;
			qsort(file, nb_file, sizeof(struct clock), compare);
			
			//renvoyer reponse positive au site
			hl.time++;
			printf(" evenement %i : ",hl.time);
			
			sprintf(chaine, "(%i,%i) autorise l'entree en SC", hl.time, hl.si);
			printf(" j'envoie a %i l'autorisation d'entree en SC ", new_clock.si);
			SendMsg(new_clock.si, chaine);
		}
		else if(strncmp("autorise", chaine2, 8)==0)
		{
			//le message est une autorisation
			accord[new_clock.si-site0]=1;
		}
		else if(strncmp("LIBERATION", chaine2, 10)==0)
		{
			//le message est une demande de liberation. Il faut enlever le premier element de file
			size=sizeof(file)/sizeof(int);
			end=file+size;
			
			for(d=file,p=file+1;p<end;++p,++d)
			{
				*d=*p;
			}
			nb_file--;
		}
    }
	
	/* Petite boucle d'attente */
    for(l=0;l<1000000;l++) { 
      t=t*3;
      t=t/3;
    }
	
	//SC
	if(nb_file!=0)
	{
		if(testAccord(accord, NSites) && PortBase==file[0].si)
		{
			//entre en SC
			printf("\nJE SUIS EN SC !\n");
			
			resetAccord(accord, NSites);
			//envoyer les messages de liberation
			hl.time++;
			printf(" evenement %i : ",hl.time);
			for(i=site0;i<site0+NSites;i++)
			{
				if(i!=PortBase)
				{
					//envoyer aux autres sites la demande
					sprintf(chaine,"(%i,%i) LIBERATION", hl.time, hl.si);
					printf(" j'envoie a %i la demande de liberation ", i);
					SendMsg(i, chaine);
				}
			}
			
			//supprimer l'element de sa propre liste
			size=sizeof(file)/sizeof(int);
			end=file+size;
			
			for(d=file,p=file+1;p<end;++p,++d)
			{
				*d=*p;
			}
			nb_file--;
			
			demande=0;
		}
	}
	
	rd=rand();
	/* rien faire dans X% des cas */
	if(rd%50==1)
	{
		//evenement locale
		hl.time++;
		printf("Evenement local %d",hl.time);
	}
	else if(rd%50==0 && !demande)
	{
		hl.time++;
		printf(" evenement %i : ",hl.time);
		//demande d'entree en Section critique si pas de demande dejà en cours
		demande=1;
		file[nb_file]=hl;
		nb_file++;
		qsort(file, nb_file, sizeof(struct clock), compare);
		accord[PortBase-site0]=1;
		
		for(i=site0;i<site0+NSites;i++)
		{
			if(i!=PortBase)
			{
				//envoyer aux autres serveurs la demande
				sprintf(chaine,"(%i,%i) REQ", hl.time, hl.si);
				printf(" j'envoie a %i la requete de SC ", i);
				SendMsg(i, chaine);
			}
		}
	}
	
    printf(".");fflush(0); /* pour montrer que le serveur est actif*/
  }

  close (s_ecoute);
  return 0;
}


