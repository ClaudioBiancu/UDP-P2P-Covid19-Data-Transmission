#include "./utility/utility_ds.c"

/********************************VARIABILI*********************************/
int sd;
int peersConnessi=0;
char buffer[BUFLEN];//buffer gestione socket
char buffer_stdin[BUFLEN]; //Buffer comandi da standard input
char recv_buffer[MAX_TIPO+1]; //Messaggio di richiesta connessione
struct sockaddr_in my_addr, cl_addr;
struct timeval *timeout;
//Variabili per gestire input da socket oppure da stdin
fd_set readset; //set di descrittori pronti
fd_set master; // set di descrittori da monitorare
int fdmax; //Descrittore max


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
}

void leggiComando() {
        char* comando;

        //Attendo input da tastiera
        fgets(buffer_stdin, BUFLEN, stdin);

        //Estraggo la prima parola digitata cosi' da poter discriminare i vari comandi
        comando = strtok(buffer_stdin, " \n");
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
        fdmax = sd;
        FD_SET(0, &master);
        printf(">");
        //ciclo infinito che gestisce il funzionameto del discoveryserver
        while(1){

                timeout = malloc(sizeof(timeout));
		timeout->tv_sec = 60;
		timeout->tv_usec = 0;

                readset = master;
                fflush(stdout);
                select(fdmax+1, &readset, NULL, NULL, timeout);

                //Gestione comando da terminale
                if(FD_ISSET(0, &readset)){
                        leggiComando();
                        printf(">");
                        FD_CLR(0, &readset);
                }


                //Gestione messaggi su socket
                if(FD_ISSET(sd, &readset)){
                        int portaPeer;
                        portaPeer=riceviUDP(sd, buffer, MAX_SOCKET_RECV);
                        sscanf(buffer, "%s", recv_buffer);
                        recv_buffer[MAX_TIPO] = '\0';

                        printf("Arrivato messaggio %s da %d sul socket\n", buffer, portaPeer);
                        if(strcmp(recv_buffer, "BOOT_RIC") == 0) {
                                int temp_port[2]; //Variabili per salvare eventuali vicini
                                char lista_buffer[MAX_LISTA]; //Buffer per invio liste al peer
                                int n; //Variabile per la lunghezza del messaggio da inviare al peer
                                char list_update_buffer[MAX_LISTA]; //Liste da inviare ai peer a cui e' cambiata la lista dei vicini

                                if(!alreadyBooted(portaPeer)){
                                        if(inserisci_peer(LOCALHOST, portaPeer, peersConnessi)<0){ //Assumo che i peer partano tutti sulla macchina locale
                                                printf("Impossibile inserire il peer\n");
                                                //Uscita
                                                FD_CLR(sd, &readset);
                                                continue;
                                        }
                                }
                                trovaVicini(portaPeer, peersConnessi+1, &temp_port[0], &temp_port[1]);
                                printf("Vicini di %d: %d e %d\n", portaPeer, temp_port[0], temp_port[1]);

                                if(temp_port[0] == -1 && temp_port[1] == -1)
                                        n = sprintf(lista_buffer, "%s", "VICINI_L");
                                else if(temp_port[1] == -1)
                                        n = sprintf(lista_buffer, "%s %d", "VICINI_L", temp_port[0]);
                                else
                                        n = sprintf(lista_buffer, "%s %d %d", "VICINI_L", temp_port[0], temp_port[1]);

                                printf("Lista da inviare a %d: %s (lunga %d byte)\n", portaPeer, lista_buffer, n);

                                inviaUDP(sd, lista_buffer, n, portaPeer);

                                //Invio eventuale lista aggiornata al primo peer
                                if(temp_port[0] != -1){
                                    //Preparo la struttura che devo tirare su
                                    trovaLista(temp_port[0], peersConnessi+1, "VIC_UPDT", list_update_buffer, &n);
                                    printf("Invio lista di update %s a %d\n", list_update_buffer, temp_port[0]);
                                    inviaUDP(sd, list_update_buffer, n, temp_port[0]);
                                }

                                if(temp_port[1] != -1){
                                    trovaLista(temp_port[1], peersConnessi+1, "VIC_UPDT", list_update_buffer, &n);
                                    printf("Invio lista di update %s a %d\n", list_update_buffer, temp_port[1]);
                                    inviaUDP(sd, list_update_buffer, n, temp_port[1]);
                                }

                                //Incremento il numero di peer
                                peersConnessi++;
                                printf("Peer connessi: %d\n\n>", peersConnessi);
                        }
                        FD_CLR(sd, &readset);
                }
        }
        return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
