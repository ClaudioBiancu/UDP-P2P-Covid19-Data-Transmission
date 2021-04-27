#include "./utility/utility_peer.c"

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
fd_set readset; //set di descrittori pronti
fd_set master; // set di descrittori da monitorare
int fdmax; //Descrittore max

/*****************************FINE VARIABILI*********************************/




/********************************SEZIONE OPERATIVA PEER*********************************/


///////////////////////////Funzioni per i COMANDI//////////////////
/*  Controlla che il peer non sia già registrato sul DS. In caso negativo controlla che i parametri siano inseriti correttamente e successivamente
    chiama le funzioni per la creazione del socket e per la registrazione sul DS.
*/
void start(char*parola){
    int porta;
    if(registrato==1){ // Il comando start e' gia' andato a buon fine precedentemente, quindi la variabile registrato e' stata settata.
        printf(" Attenzione il peer è già stato avviato correttamente\n\n");
        return;
    }

    parola = strtok(NULL, SPAZIO);
    if(parola==NULL || strlen(parola) > 45){// Controllo che numero di caratteri sia 45 di un indirizzo ipv6
        fprintf(stderr, "L'indirizzo ip non e' valido. Un indirizzo ipv4 puo' avere fino a 15 caratteri, un indirizzo ipv6 fino a 45\n");
        return;             // Comunque in caso di errore, questo  verra' segnalato nel momento di tentativo di connessione al server
    }

    strcpy(buffer, parola);  //La prima parola dopo il comando rappresenta l'indirizzo ip del DS
    parola = strtok(NULL, SPAZIO);
    if(parola!=NULL){
        porta= atoi(parola); //La seconda parola dopo il comando rappresenta la porta di ascolto del DS
        if(porta>0 || porta<65535)//controllo che la porta inserita sia valida
            myInfo.porta=porta;
        else{
            printf("numero porta %d:", porta);
            fprintf(stderr, "Il numero di porta inserita non e' utilizzabile, dovrebbe essere compresa tra 0 e 65535\n");
            return;
        }
    }
    else{
        fprintf(stderr, "Il numero di porta inserita non e' utilizzabile, dovrebbe essere compresa tra 0 e 65535\n");
        return;
    }
    printf(" Qui registro al ds\n\n");

    //registrazione():// chiedo il boot al DS
}

void add(){
    printf("\nADD AVVIATO\n\n");
}

void get(){
    printf("\nGET AVVIATO\n\n");
}

void stop(){
    printf("\nSTOP AVVIATO\n\n");
}


/******************************** FINE SEZIONE OPERATIVA PEER*********************************/



/********************************SEZIONE GRAFICA TERMINALE*********************************/


void guidaPeer(char* comando){
    if(comando==NULL){
        printf(" COMANDI DISPONIBILI:\n");
        printf(" 1) !start <indirizzo DS> <porta DS> \t Richiedi la connessione al Discovery Server\n");
        printf(" 2) !add <type> <quantity>           \t Aggiungi una entry relativa all'evento *type* con quantità *quantity* \n");
        printf(" 3) !get <aggr> <type> <period>      \t Ottieni il dato aggregato sui dati relativi a un lasso temporale sulle entry di tipo *type* \n");
        printf(" 4) !stop                            \t Permette di uscire dal programma\n");
    }
  }

void interfacciaPeerStart()
{
    printf("\n****************************************************** Peer Covid-19 ******************************************************\n\n");
    guidaPeer(NULL);
    printf("\n***************************************************************************************************************************\n");
    return;
}

void leggiComando() {
    char* parola;
    /*Attendo input da tastiera*/
    fgets(buffer, BUFLEN, stdin);

    /*Estraggo la prima parola digitata cosi' da poter discriminare i vari comandi*/
    parola = strtok(buffer, " \n");
    if(strcmp(parola, "!start") == 0) {
        start(parola);
    }
    else if(strcmp(parola, "!add") == 0) {
        add(parola);
    }
    else if(strcmp(parola, "!get") == 0) {
        get(parola);
    }
    else if(strcmp(parola, "!stop") == 0) {
        stop(parola);
    }
    else {
        printf("Comando non valido.\n\n ");
        guidaPeer(NULL);
    }


}

/********************************FINE SEZIONE GRAFICA TERMINALE*********************************/



/************************************** SEZIONE MAIN*********************************/


int main(int argc, char* argv[]){
    int porta;
    if(argc != 2) {//Controllo parametri
        fprintf(stderr, "Istruzione di avvio non riconosciuta, dovresti usare: ./peer <porta>\n");
        exit(1);
    }


    porta=atoi(argv[1]);
    if(porta<0 || porta>65535){//controllo che la porta inserita sia valida
        fprintf(stderr, "Il numero di porta inserita non e' utilizzabile, dovrebbe essere tra 0 e 65535\n");
        exit(1);
    }



  //  Creo il socket d'ascolto UDP con il quale il peer comunichera' con il DS
    sd=creaSocketAscolto(&socketAscolto_addr, porta);
    //inizializzo le informazione del mio peer, che non si è ancora registrato ed avra' i vicini a -1
    myInfo.porta=porta;
    myInfo.vicino1=-1;
    myInfo.vicino2=-1;

    //Registrato verra' posto a uno se il comando start andra' a buon fine
    registrato=0;
    interfacciaPeerStart();
    printf("> ");
    //Gestisco variabili per la select
    FD_ZERO(&master);
    FD_ZERO(&readset);

    FD_SET(sd, &master);
    FD_SET(0, &master);
    fdmax = sd;
    //ciclo infinito che gestisce il funzionameto del peer
    while(1){
        readset = master;
        printf("> ");
        select(fdmax+1, &readset, NULL, NULL, NULL);


        if(FD_ISSET(0, &readset)){
            leggiComando();

            FD_CLR(0, &readset);
       }

        if(FD_ISSET(sd, &readset)){

          FD_CLR(sd, &readset);
        }
    }
return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
