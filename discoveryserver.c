#include "./utility/utility_ds.c"



/********************************SEZIONE OPERATIVA DISCOVERY SERVER****************************/


void showpeers(){
        stampaPeers(peersConnessi);
}

void showneighbor(char*parola){
        int porta;
        if(peersConnessi == 0){
                printf("Nessun peer connesso\n");
                return;
        }
        parola = strtok(NULL, SPAZIO);
        if(parola==NULL){
                stampaTuttiVicini(peersConnessi);
        }
        else{
                porta= atoi(parola); //La seconda parola dopo il comando rappresenta la porta del peer di cui si vogliono stampare i vicini
                if(porta>0 || porta<65535){//controllo che la porta inserita sia valida
                        stampaVicino(peersConnessi, porta);
                }
                else{
                        fprintf(stderr, "Il numero di porta inserita non e' utilizzabile, dovrebbe essere compresa tra 0 e 65535\n\n");
                        return;
                }
        }
}

void esc(){
        //Invia a tutti i peer un messaggio
        int i;
        printf("Invio serie di messaggi SRV_EXIT\n");
        for(i=0; i<peersConnessi; i++){
            //Invia al peer il messaggio di exit
            inviaUDP(sd, "SRV_EXIT", MAX_TIPO, trovaPorta(i));
        }
        //Aggiorno il numero di peer connessi (dovrebbe essere 0)
        peersConnessi = 0;
        //Cancello il file con la lista di peer
        remove("./txtDS/bootedPeers.txt");


        close(sd);
        _exit(0);

}

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

void help(){
        guidaServer(NULL);
}

void interfacciaServerStart()
{
        printf("\n********************* DISCOVERY SERVER Covid-19 ***********************\n\n");
        guidaServer(NULL);
        printf("\n**********************************************************************\n\n");
}



void leggiComando() {
        char* parola;

        //Attendo input da tastiera
        fgets(buffer, BUFLEN, stdin);

        //Estraggo la prima parola digitata cosi' da poter discriminare i vari comandi
        parola = strtok(buffer, SPAZIO);
        if(parola==NULL){
                printf("Comando non valido.\n\n ");
                guidaServer(NULL);
        }
        //Controllo che la parola digitata sia uguale a uno dei comandi disponibili e in caso chiamo la funzione associata
        if(strcmp(parola, "!help") == 0) {
                help(parola);
        }
        else if(strcmp(parola, "!showpeers") == 0) {
                showpeers();
        }
        else if(strcmp(parola, "!showneighbor") == 0) {
                showneighbor(parola);
        }
        else if(strcmp(parola, "!esc") == 0) {
                esc();
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
        peersConnessi=0;
        EntriesGiornaliere=0;
        //Gestisco variabili per la select
        FD_ZERO(&master);
        FD_ZERO(&readset);
        FD_SET(sd, &master);
        fdmax = sd;
        FD_SET(0, &master);
        printf(">");
        //ciclo infinito che gestisce il funzionameto del discoveryserver
        while(1){


                readset = master;
                fflush(stdout);

                timeout = malloc(sizeof(timeout));
                timeout->tv_sec = 15;
                timeout->tv_usec = 0;

                select(fdmax+1, &readset, NULL, NULL, timeout);

		checkTime();

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
                                else{
                                        printf("Peer gia' inserito: %d\n\n>");
                                }
                        }

                        if(strcmp(recv_buffer, "CLT_EXIT") == 0){
                                //Variabili per salvare informazioni temporanee
                                int temp_nbr_port[2];
                                char list_update[MAX_LISTA];
                                int n;

                                printf("Ricevuto messaggio di richiesta di uscita da %d\n", portaPeer);

                                //Se il peer per qualche motivo non e' in lista non faccio nulla
                                if(!alreadyBooted(portaPeer)){
                                        printf("Peer %d non presente nella lista dei peer connessi. Uscita\n", portaPeer);
                                        FD_CLR(sd, &readset);
                                        continue;
                                }

                                trovaVicini(portaPeer, peersConnessi, &temp_nbr_port[0], &temp_nbr_port[1]);

                                printf("Elimino il peer dalla rete. Aggiorno la lista di vicini di %d e %d\n", temp_nbr_port[0], temp_nbr_port[1]);

                                rimuoviPeer(portaPeer);

                                if(temp_nbr_port[0] == -1){
                                        printf("Eliminato l'ultimo peer connesso\n\n>");
                                }
                                else {
                                        trovaLista(temp_nbr_port[0], peersConnessi-1, "VIC_UPDT", list_update, &n);
                                        printf("Lista che sta per essere inviata a %d: %s\n\n>", temp_nbr_port[0], list_update);
                                        inviaUDP(sd, list_update, n, temp_nbr_port[0]);

                                        if(temp_nbr_port[1] != -1){
                                                trovaLista(temp_nbr_port[1], peersConnessi-1, "VIC_UDPT", list_update, &n);
                                                printf("Lista che sta per essere inviata a %d: %s\n\n>", temp_nbr_port[1], list_update);
                                                inviaUDP(sd, list_update, n, temp_nbr_port[1]);
                                        }

                                }
                        peersConnessi--;
                        }

                        if(strcmp(recv_buffer, "NEW_ENTR") == 0){
                                printf("Controllo NEW_ENTR");
                                int n;
                                int t;
                                sscanf(buffer, "%s %i %i", recv_buffer, &n, &t);
                                DS_entry.num_entry_N += n;
                                DS_entry.num_entry_T += t;
				EntriesGiornaliere ++;


                        }

                        if(strcmp(recv_buffer, "ENTR_REQ") == 0){
                                int tipo;
                                char bound1[MAX_DATA];
                                char bound2[MAX_DATA];


                                sscanf(buffer, "%s %i %s %s", recv_buffer, &tipo, bound1, bound2);
                                leggiEntries(tipo, bound1, bound2, portaPeer);

                        }
                }
                FD_CLR(sd, &readset);
        }
        return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
