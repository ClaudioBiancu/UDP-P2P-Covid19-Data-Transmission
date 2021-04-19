

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
//#include "./utility/utility_peer.c"

#define SPAZIO " \n"
//Dimensione massima del buffer
#define BUFLEN 1024

#define LOCALHOST "127.0.0.1"

/********************************VARIABILI*********************************/
//socket di ascolto del peer
int sd;//socket di ascolto del peer
int len; //variabile di servizio
char buffer[BUFLEN]; //buffer utilizzato dal socket
char buffer_stdin[BUFLEN]; //buffer utilizzato per i comandi da standard-input
struct sockaddr_in socketAscolto_addr; // Struttura per gestire il socket di ascolto del peer
struct sockaddr_in sv_addr; // Struttura per il server
int registrato; //se a 1 il ds ha risposto alla richiesta di boot

//struttura che descrive il mio peer
struct peer{
    int porta;
    int vicino1;
    int vicino2;
} myInfo;

//Variabili per gestire input da socket oppure da stdin
fd_set readset, master;
int fdmax;

/*****************************FINE VARIABILI*********************************/




/********************************SEZIONE OPERATIVA PEER*********************************/


///////////////////////////Funzioni per i COMANDI//////////////////
/*  Controlla che il peer non sia già registrato sul DS. In caso negativo controlla che i parametri siano inseriti correttamente e successivamente
    chiama le funzioni per la creazione del socket e per la registrazione sul DS.
*/


/********************************FINE SEZIONE GRAFICA TERMINALE*********************************/



/************************************** SEZIONE MAIN*********************************/


int main(int argc, char* argv[]){
    printf("Prima del fd_set: %d ", sd);
    /*int porta;
    if(argc != 2) {//Controllo parametri
        fprintf(stderr, "Istruzione di avvio non riconosciuta, dovresti usare: ./peer <porta>\n");
        exit(1);
    }


    porta=atoi(argv[1]);
    if(porta<0 || porta>65535){//controllo che la porta inserita sia valida
        fprintf(stderr, "Il numero di porta inserita non e' utilizzabile, dovrebbe essere tra 0 e 65535\n");
        exit(1);
    }


    FD_ZERO(&master);
    FD_ZERO(&readset);
    //Creo il socket d'ascolto UDP con il quale il peer comunichera' con il DS
    //sd=creaSocketAscolto(&socketAscolto_addr, porta);
    sd=0;
    //inizializzo le informazione del mio peer, che non si è ancora registrato ed avra' i vicini a -1
    myInfo.porta=porta;
    myInfo.vicino1=-1;
    myInfo.vicino2=-1;

    //Registrato verra' posto a uno se il comando start andra' a buon fine
    registrato=0;
    //interfacciaPeerStart();*/
    //Gestisco variabili per la select
    printf("Prima del fd_set: %d ", sd);
    FD_SET(sd, &master);
    FD_SET(0, &master);
    fdmax = sd;
    printf("Prima del While: %d ", sd);
    //ciclo infinito che gestisce il funzionameto del peer
    while(1){
        readset = master;
        printf("> ");
      //  select(fdmax+1, &readset, NULL, NULL, NULL);

        if(FD_ISSET(0, &readset)){
            //leggiComando();
            //printf("dentro if: %d ", sd);
        }

        //if(FD_ISSET(sd, &readset)){
    }//
return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
