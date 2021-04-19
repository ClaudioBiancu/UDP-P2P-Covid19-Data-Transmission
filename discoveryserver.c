#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#define BUFLEN 1024

int sd, len, attivo;
int num_peers=0;
char buffer[BUFLEN];
struct sockaddr_in my_addr, cl_addr;

struct peerReg{
    struct sockaddr_in peer_addr;
    struct sockaddr_in precPeer;
    struct sockaddr_in nextPeer;
} *registroPeers;


void creaSocketAscolto(int porta) {
  int ret;
  int addrlen = sizeof(cl_addr);
  /* Creazione socket UDP */
  sd = socket(AF_INET, SOCK_DGRAM|SOCK_NONBLOCK, 0);
  /* Creazione indirizzo */
  memset(&my_addr, 0, sizeof(my_addr)); // Per convenzione
  my_addr.sin_family = AF_INET ;
  my_addr.sin_port = htons(porta);
  my_addr.sin_addr.s_addr = INADDR_ANY;

  /*Aggancio*/
  ret = bind(sd, (struct sockaddr*)&my_addr, sizeof(my_addr));
  if( ret < 0 ){
      perror("Bind non riuscita\n");
      exit(0);
  }
  printf("Discovery Server in ascolto sulla porta:%d\n", porta);
 /* while(1){
      do{
        ret = recvfrom(sd, buffer, BUFLEN, 0,(struct sockaddr*)&cl_addr, &addrlen);
        buffer[BUFLEN]='\0';
        printf("%s\n", buffer);
        if(ret<0){

        }
}*/

}




/********************************SEZIONE GRAFICA TERMINALE*********************************/

void guidaServer(char* comando){
if(comando==NULL){
      printf(" COMANDI DISPONIBILI:\n");
      printf(" 1) !help               \t Mostra i dettagli dei comandi\n");
      printf(" 2) !showpeers          \t Mostra un elenco dei peer connessi\n");
      printf(" 3) !showneighbor <peer>\t Mostra i neighbor di un peer\n");
      printf(" 4) !esc                \t Chiude il DSDettaglio comandi\n");
  }
}

void interfacciaServerStart()
{
    printf("\n********************* DISCOVERY SERVER Covid-19 ***********************\n\n");
    guidaServer(NULL);
    printf("\n**********************************************************************\n\n");
}

void help(){
    printf("\n HELP AVVIATO\n\n");
}

void showpeers(){
    printf("\n SHOWPEERS AVVIATO\n\n");
}

void showneighbor(){
    printf("\nSHOWNEIGHBOR AVVIATO\n\n");
}

void esc(){
    printf("\nESC AVVIATO\n\n");
    attivo=0;
}

void leggiComando() {
    char* parola;

    printf("> ");

    //Attendo input da tastiera
    fgets(buffer, BUFLEN, stdin);

    //Estraggo la prima parola digitata cosi' da poter discriminare i vari comandi
    parola = strtok(buffer, " \n");
    //Controllo che la parola digitata sia uguale a uno dei comandi disponibili e in caso chiamo la funzione associata
    if(strcmp(parola, "!help") == 0) {
        help(parola);
    }
    else if(strcmp(parola, "!showpeers") == 0) {
        showpeers(parola);
    }
    else if(strcmp(parola, "!showneighbor") == 0) {
        showneighbor(parola);
    }
    else if(strcmp(parola, "!esc") == 0) {
        esc(parola);
    }
    else {
        printf("Comando non valido.\n\n ");
        guidaServer(NULL);
    }
}

/********************************FINE SEZIONE GRAFICA TERMINALE*********************************/


/************************************** SEZIONE MAIN*********************************/

int main(int argc, char* argv[]){
  if(argc != 2) {
      fprintf(stderr, "Istruzione di avvio non riconosciuta, dovresti usare: ./discoveryserver <porta>\n");
      exit(1);
  }
  interfacciaServerStart();

  creaSocketAscolto(atoi(argv[1]));

  attivo=1;
  //Dopo aver stampato la lista dei comandi attende l'immissione di uno di essi
  while(attivo!=0){
      leggiComando();
      printf("\n");
  }
  return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
