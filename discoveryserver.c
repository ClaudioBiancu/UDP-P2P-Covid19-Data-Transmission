#include "./utility/utility_ds.c"

/********************************VARIABILI*********************************/
int sd;
int num_peers=0;
char buffer[BUFLEN];//buffer gestione socket
char buffer_stdin[BUFLEN]; //Buffer comandi da standard input
char recv_buffer[MAX_TIPO+1]; //Messaggio di richiesta connessione
struct sockaddr_in my_addr, cl_addr;

//Variabili per gestire input da socket oppure da stdin
fd_set readset; //set di descrittori pronti
fd_set master; // set di descrittori da monitorare
int fdmax; //Descrittore max

struct peerReg{
    struct sockaddr_in peer_addr;
    struct sockaddr_in precPeer;
    struct sockaddr_in nextPeer;
} *registroPeers;

/*****************************FINE VARIABILI*********************************/

/********************************SEZIONE OPERATIVA DISCOVERY SERVER****************************/


/******************************** FINE SEZIONE OPERATIVA PEER*********************************/



/********************************SEZIONE GRAFICA TERMINALE*********************************/

void guidaServer(char* comando){
if(comando==NULL){
      printf(" COMANDI DISPONIBILI:\n");
      printf(" 1) !help               \t Mostra i dettagli dei comandi\n");
      printf(" 2) !showpeers          \t Mostra un elenco dei peer connessi\n");
      printf(" 3) !showneighbor <peer>\t Mostra i neighbor di un peer\n");
      printf(" 4) !esc                \t Chiude il DSDettaglio comandi\n\n");
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
    char* comando;

    //Attendo input da tastiera
    fgets(buffer_stdin, BUFLEN, stdin);

    //Estraggo la prima parola digitata cosi' da poter discriminare i vari comandi
    parola = strtok(buffer_stdin, " \n");
    //Controllo che la parola digitata sia uguale a uno dei comandi disponibili e in caso chiamo la funzione associata
    if(strcmp(comando, "!help") == 0) {
        help(comando);
    }
    else if(strcmp(comando, "!showpeers") == 0) {
        showpeers(comando);
    }
    else if(strcmp(comando, "!showneighbor") == 0) {
        showneighbor(comando);
    }
    else if(strcmp(comando, "!esc") == 0) {
        esc(comando);
    }
    else {
        printf("\n Comando non valido.\n\n ");
        guidaServer(NULL);
    }
}

/********************************FINE SEZIONE GRAFICA TERMINALE*********************************/


/************************************** SEZIONE MAIN*********************************/

int main(int argc, char* argv[]){
  int porta;
  if(argc != 2) {
      fprintf(stderr, " Istruzione di avvio non riconosciuta, dovresti usare: ./discoveryserver <porta>\n");
      exit(1);
  }

  porta=atoi(argv[1]);
  if(porta<0 || porta>65535){//controllo che la porta inserita sia valida
      fprintf(stderr, " Il numero di porta inserita non e' utilizzabile, dovrebbe essere tra 0 e 65535\n");
      exit(1);
  }
  interfacciaServerStart();
  sd=creaSocketAscolto(&my_addr, porta);


  //Gestisco variabili per la select
  FD_ZERO(&master);
  FD_ZERO(&readset);

  FD_SET(sd, &master);
  FD_SET(0, &master);
  fdmax = sd;
  //ciclo infinito che gestisce il funzionameto del discoveryserver
  while(1){
      readset = master;
      printf(">");
      fflush(stdout);
      select(fdmax+1, &readset, NULL, NULL, NULL);


      if(FD_ISSET(0, &readset)){
          leggiComando();

          FD_CLR(0, &readset);
     }

      if(FD_ISSET(sd, &readset)){
        riceviUDP(sd, buffer, MAX_SOCKET_RECV);
        sscanf(buffer, "%s", recv_buffer);
        recv_buffer[MAX_TIPO] = '\0';

        printf("Arrivato messaggio %s da %d sul socket\n", sd, peer_port);


        FD_CLR(sd, &readset);
      }
  }
  return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
