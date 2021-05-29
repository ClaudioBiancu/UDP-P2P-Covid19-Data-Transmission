#include "./utility/utility_peer.c"

/********************************VARIABILI*********************************/
//socket di ascolto del peer
int sd;//socket di ascolto del peer
int ret; //variabile di servizio
char buffer[BUFLEN]; //buffer utilizzato dal socket
char buffer_stdin[BUFLEN]; //buffer utilizzato per i comandi da standard-input
struct sockaddr_in socketAscolto_addr; // Struttura per gestire il socket di ascolto del peer
int portaServer;

//struttura che descrive il mio peer


//Variabili per gestire input da socket oppure da stdin
fd_set readset; //set di descrittori pronti
fd_set master; // set di descrittori da monitorare
int fdmax; //Descrittore max

struct Peer{
        int porta;
        int vicino1;
        int vicino2;
} myInfo;

/*****************************FINE VARIABILI*********************************/




/********************************SEZIONE OPERATIVA PEER*********************************/


///////////////////////////Funzioni per i COMANDI//////////////////
/*  Controlla che il peer non sia già registrato sul DS. In caso negativo controlla che i parametri siano inseriti correttamente e successivamente
    chiama le funzioni per la creazione del socket e per la registrazione sul DS.
*/
void start(char*parola){
        int porta;
        int util_porta;
        char recv_buffer[MAX_LISTA];
        char temp_buffer[MAX_TIPO];
        int temp_n[2];
        if(portaServer!=-1){ // Il comando start e' gia' andato a buon fine precedentemente
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
                        portaServer=porta;
                else{
                        printf("numero porta %d:", porta);
                        fprintf(stderr, "Il numero di porta inserita non e' utilizzabile, dovrebbe essere compresa tra 0 e 65535\n\n");
                        return;
                }
        }
        else{
                fprintf(stderr, "Il numero di porta inserita non e' utilizzabile, dovrebbe essere compresa tra 0 e 65535\n\n");
                return;
        }
        inviaUDP(sd, "BOOT_RIC", MAX_SOCKET_RECV, portaServer);
        util_porta=riceviUDP(sd, recv_buffer, MAX_LISTA);
        while(util_porta!=portaServer){
                inviaUDP(sd, "BOOT_RIC", MAX_SOCKET_RECV, portaServer);
                printf("Attendo Risposta dal Discovery Server\n\n");
                sleep(ATTESA_BOOT);
                util_porta=riceviUDP(sd, recv_buffer, MAX_LISTA);
        }
        ret = sscanf(recv_buffer, "%s %d %d", temp_buffer, &temp_n[0], &temp_n[1]);

        switch(ret){
                    case 1:
                        printf("Lista vuota, nessun vicino\n\n");
                        break;
                    case 2:
                        printf("Un vicino con porta %d\n\n", temp_n[0]);
                        myInfo.vicino1 = temp_n[0];
                        break;
                    case 3:
                        printf("Due vicini con porta %d e %d\n\n", temp_n[0], temp_n[1]);
                        myInfo.vicino1 = temp_n[0];
                        myInfo.vicino2 = temp_n[1];
                        break;
                    default:
                        printf("Problema nella trasmissione della lista\n\n");
                }

}

void add(char*parola){
        char tipo;
        int quanto;
        char nuovaEntry[TIPO_ENTRY+1];

        if(portaServer == -1){//Se peer non connesso non faccio nulla
                printf("Peer non connesso\n");
                return;
        }

        parola = strtok(NULL, SPAZIO);
        if(parola==NULL){
                printf("Formato del comando non valido \n\n>");
                return;
        }
        if((strcmp(parola, "N") != 0) && (strcmp(parola, "T") != 0)){
		printf("Formato invalido, il tipo puo' essere solamente \"N\" o \"T\" \n");
                return;
        }
        if(strcmp(parola, "N") == 0)
                tipo='N';
        else
                tipo='T';
        parola = strtok(NULL, SPAZIO);
        if(parola==NULL){
                printf("Formato del comando non valido \n\n>");
                return;
        }

        quanto= atoi(parola);
        if(quanto < 1){
                printf("Formato invalido, inserisci una quantità positiva\n");
                return;
        }


        ret = sprintf(nuovaEntry, "%s %c", "NEW_ENTR", tipo);
        nuovaEntry[ret] = '\0';

        inviaUDP(sd, nuovaEntry, ret, portaServer);
        inserisciEntry(tipo, quanto, myInfo.porta);

}

void get(char*parola){
        char aggr;
        char tipo;
        char bound[2][MAX_DATA+1];
        int ret=3; //Inizialmente lo uso per gestire il numero dei parametri, che minimo sono 3
        int tot_entr;
        int peer_entr;
        int sum_entr;
        int temp_buffer[6];
        char get_buffer[MAX_SOMMA]; //Ricevere il messaggio
        char util_buffer[MAX_TEMPO];
        char altro_buffer[MAX_SOMMA];
        char r_type;
        int r_quantity;
        char entry_ins_buffer[MAX_PEERS*6];
        char control_buffer[7];
        char* isIn;

        //Se peer non connesso non faccio nulla
        if(portaServer == -1){
                printf("Peer non connesso\n");
                return;
        }
        parola = strtok(NULL, SPAZIO);
        if(parola==NULL){
                printf("Formato del comando non valido \n\n");
                return;
        }
        if((strcmp(parola, "t") != 0) && (strcmp(parola, "v") != 0)){
		printf("Aggregato  invalido, puo' essere solamente totale \"t\" o variazione \"v\" \n");
                return;
        }
        else{
                if(strcmp(parola, "t") == 0)
                        tipo='t';
                else
                        tipo='v';
        }
        parola = strtok(NULL, SPAZIO);
        if(parola==NULL){
                printf("Formato del comando non valido \n\n");
                return;
        }
        if((strcmp(parola, "N") != 0) && (strcmp(parola, "T") != 0)){
		printf("Formato invalido, il tipo puo' essere solamente \"N\" o \"T\" \n");
                return;
        }
        else{
                if(strcmp(parola, "N") == 0)
                        tipo='N';
                else
                        tipo='T';
        }
        parola = strtok(NULL, SPAZIO);
        if (parola!=NULL){ //L'utente ha inserito correttamente i parametri 3 e 4 relativi alle date bound
                ret=5;// I dati sono 5
                strcpy(bound[0], parola);
                parola = strtok(NULL, SPAZIO);
                if(parola==NULL){
                        printf("Formato del comando non valido \n\n");
                        return;
                }
                strcpy(bound[1], parola);
                bound[0][MAX_DATA] = '\0';
                bound[1][MAX_DATA] = '\0';
                if(!controllaDate(bound[0], bound[1], aggr))
                        return;
                else{
                        printf("Formato date corretto \n");
                }
        }
        else{ //L'utente non ha inserito un periodo
                strcpy(bound[0], "*");
                strcpy(bound[1], "*");
                if(!controllaDate(bound[0], bound[1], aggr))
                        return;
                else{
                        printf("Formato date corretto \n");
                }
        }



        sum_entr = 0;
        //Invio richiesta al server
        ret = sprintf(get_buffer, "%s %c", "ENTR_REQ", tipo);
        get_buffer[ret] = '\0';
        inviaUDP(sd, get_buffer, ret, portaServer);

        //Ricevo risposta
        riceviUDP(sd, get_buffer, MAX_ENTRY);
        sscanf(get_buffer, "%s %d", temp_buffer, &tot_entr);

        peer_entr = contaEntries(tipo, myInfo.porta);

        printf("Entries nel server: %d; entries qui: %d\n", tot_entr, peer_entr);

        //Se nessuna entry, inutile continuare
        if(!tot_entr)
                printf("Nessun dato aggregato da calcolare\n");

        //Se ho tutti i dati che servono, eseguo il calcolo e lo scrivo
        else if(tot_entr == peer_entr){
                sum_entr = sommaEntries(tipo, myInfo.porta);
                scriviAggr(tot_entr, sum_entr, tipo, myInfo.porta);
        }

        //Altrimenti
        else {
                //Se non ho vicini non posso calcolare nulla
                if(myInfo.vicino1 == -1 && myInfo.vicino2 == -1)
                        printf("Errore insolubile, impossibile calcolare il dato richiesto\n");

                else {
                        printf("Devo chiedere informazioni ai miei vicini\n");
                        //Controllo se qualche peer ha il dato aggregato pronto

                        //Posso riciclare il buffer get_buffer
                        ret = sprintf(get_buffer, "%s %d %d %c", "AGGR_REQ", myInfo.porta, tot_entr, tipo);
                        get_buffer[ret] = '\0';

                        inviaUDP(sd, get_buffer, ret, myInfo.vicino1);

                        //Posso sfruttare get_buffer
                        riceviUDP(sd, get_buffer, MAX_SOMMA);

                        printf("Ricevuto %s\n", get_buffer);


                        sscanf(get_buffer, "%s %d", temp_buffer, &sum_entr);

                        if(sum_entr != 0){
                                printf("Ho ottenuto il dato che cercavo\n");
                                scriviAggr(tot_entr, sum_entr, tipo, myInfo.porta);
                        }
                        else{
                                printf("Nessuno ha gia' pronto il dato cercato\n");
                                //Necessario inviare a tutti la richiesta di nuove entries
                                ret = sprintf(get_buffer, "%s %d %c", "EP2P_REQ", myInfo.porta, tipo);
                                get_buffer[ret] = '\0';
                                printf("Faccio partire il giro di richieste di entries\n");
                                inviaUDP(sd, get_buffer, ret, myInfo.vicino1);

                                //Aspetto tutte le entries che mi mancano
                                while(peer_entr < tot_entr){
                                        //Nuove entries: possono arrivare da tutti
                                        ret = (myInfo.vicino1 != -1) ? 1 : 0;
                                        riceviUDP(sd, altro_buffer, MAX_ENTRY);
                                        printf("Ricevuta entry %s\n", altro_buffer);
                                        sscanf(altro_buffer, "%s %s %c %d %s", temp_buffer, util_buffer, &r_type, &r_quantity, entry_ins_buffer);
                                        //Controlli: non dovrebbero mai fallire
                                        ret = sprintf(control_buffer, "%d;", myInfo.porta);
                                        control_buffer[ret] = '\0';
                                        printf("Stringa di controllo: %s\n", control_buffer);
                                        isIn = strstr(entry_ins_buffer, control_buffer);
                                        if(!(r_type == tipo && isIn != NULL)){
                                                printf("Errore insolubile nella ricezione, arrivato pacchetto corrotto\n");
                                                continue;
                                        }
                                        ret = sprintf(altro_buffer, "%s %c %d %s", util_buffer, r_type, r_quantity, entry_ins_buffer);
                                        altro_buffer[ret] = 0;
                                        if(!entryPresente(altro_buffer, myInfo.porta)){
                                            inserisciEntryStringa(altro_buffer, myInfo.porta);
                                            peer_entr++;
                                        }
                                }


                                printf("Tutte le entries necessarie sono arrivate a destinazione\n");
                                //Eseguo il calcolo
                                sum_entr = sommaEntries(tipo, myInfo.porta);
                                //Lo riporto sul file
                                scriviAggr(tot_entr, sum_entr, tipo, myInfo.porta);
                        }
                }
        }
}

void stop(){
        //Controllo che la connessione esista
        if(portaServer== -1)
            printf("Il peer non e' connesso al DS. Uscita\n");

        else{
            //Invio eventuali entries mancanti agli eventuali vicini
           inviaEntriesVicini(sd, myInfo.porta, myInfo.vicino1, myInfo.vicino2);

            inviaUDP(sd, "CLT_EXIT", MAX_TIPO, portaServer);
        }
        printf("Peer disconnesso correttamente. Uscita\n");
        close(sd);
        exit(0);
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

void interfacciaPeerStart(){
        printf("\n****************************************************** Peer Covid-19 ******************************************************\n\n");
        guidaPeer(NULL);
        printf("\n***************************************************************************************************************************\n");
        return;
}

void leggiComando() {
        char* parola;
        fgets(buffer, BUFLEN, stdin);/*Attendo input da tastiera*/
        parola = strtok(buffer, SPAZIO);/*Estraggo la prima parola digitata cosi' da poter discriminare i vari comandi*/
        if(parola==NULL){
                printf("Comando non valido.\n\n ");
                guidaPeer(NULL);
        }
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

        //Creo il socket d'ascolto UDP con il quale il peer comunichera' con il DS
        sd=creaSocketAscolto(&socketAscolto_addr, porta);
        //inizializzo le informazione del mio peer, che non si è ancora registrato ed avra' i vicini a -1
        myInfo.porta=porta;
        myInfo.vicino1=-1;
        myInfo.vicino2=-1;

        //Registrato verra' posto a uno se il comando start andra' a buon fine
        portaServer = -1;
        interfacciaPeerStart();

        //Gestisco variabili per la select
        FD_ZERO(&master);
        FD_ZERO(&readset);
        FD_SET(sd, &master);
        FD_SET(0, &master);
        fdmax = sd;
        printf("> ");
        //ciclo infinito che gestisce il funzionameto del peer
        while(1){
                readset = master;
                //printf("> ");
                fflush(stdout);
                select(fdmax+1, &readset, NULL, NULL, NULL);

                if(FD_ISSET(0, &readset)){//Gestione comando da terminale
                        leggiComando();
                        printf("> ");
                        FD_CLR(0, &readset);
                }

                if(FD_ISSET(sd, &readset)){//Gestione messaggi su socket
                        int util_port;
                        char temp_buffer[MAX_TIPO+1];

                        util_port = riceviUDP(sd, buffer, MAX_SOCKET_RECV);
                        //Leggo il tipo del messaggio
                        sscanf(buffer, "%s", temp_buffer);
                        temp_buffer[MAX_TIPO] = '\0';

                        if(util_port == portaServer){
                                printf("Messaggio ricevuto dal server: %s\n", buffer);

                                //Arrivo nuova lista
                                if(strcmp(temp_buffer, "VIC_UPDT")==0){
                                        //Numero di nuovi neighbor
                                        int count;
                                        int temp_n[2];



                                        temp_n[0] = -1;
                                        temp_n[1] = -1;

                                        printf("Parametri prima di sscanf: %d e %d\n", temp_n[0], temp_n[1]);

                                        count = sscanf(buffer, "%s %d %d", temp_buffer, &temp_n[0], &temp_n[1]);

                                        printf("Parametri dopo sscanf: %d e %d\n", temp_n[0], temp_n[1]);

                                        //Modifica ai vicini
                                        switch(count){
                                                case 1:
                                                    //DEBUG
                                                    printf("Sono rimasto l'unico peer\n\n>");
                                                    myInfo.vicino1 = -1;
                                                    myInfo.vicino2 = -1;
                                                    break;
                                                case 2:
                                                    printf("Un vicino con porta %d\n\n>", temp_n[0]);
                                                    myInfo.vicino1  = temp_n[0];
                                                    myInfo.vicino2 = -1;
                                                    break;
                                                case 3:
                                                    printf("Due vicini con porta %d e %d\n\n>", temp_n[0], temp_n[1]);
                                                    myInfo.vicino1  = temp_n[0];
                                                    myInfo.vicino2 = temp_n[1];
                                                    break;
                                                default:
                                                    printf("Questa riga di codice non dovrebbe mai andare in esecuzione\n\n>");
                                                    break;
                                        }
                                }

                                //Arrivo chiusura Server
                                if(strcmp(temp_buffer, "SRV_EXIT")==0){
                                    printf("Il server sta per chiudere\n");

                                    //Chiude
                                    printf("Chiusura peer\n");
                                    close(sd);
                                    _exit(0);
                                }
                        }


                        else if(util_port == myInfo.vicino1 || util_port == myInfo.vicino2){
                                printf("Messaggio %s arrivato dal vicino %d\n", buffer, util_port);
                                //Vicino chiede se esiste dato aggregato
                                if(strcmp(temp_buffer, "AGGR_REQ") == 0){
                                        int req_port;
                                        int act_entries;
                                        char tipo;
                                        int aggr;
                                        char answer[MAX_ENTRY];

                                        //Invio ack


                                        //Leggo il messaggio
                                        sscanf(buffer, "%s %d %d %c", temp_buffer, &req_port, &act_entries, &tipo);

                                        aggr = controllaAggr(act_entries, tipo, myInfo.porta);

                                        if(aggr == 0){
                                                printf("Non ho la risposta alla richiesta\n");
                                                if(myInfo.vicino2 == -1){
                                                        if(myInfo.vicino1 != req_port)
                                                                printf("Errore insolubile, crash della rete\n");
                                                        else {
                                                                printf("Invio direttamente la risposta negativa a %d\n", myInfo.vicino1);
                                                                strcpy(answer, "AGGR_REP 0");
                                                                inviaUDP(sd, answer, strlen(answer), myInfo.vicino1);
                                                        }
                                                }
                                                else {
                                                        if(myInfo.vicino1 == req_port){
                                                                printf("Invio risposta negativa a %d", myInfo.vicino1);
                                                                strcpy(answer, "AGGR_REP 0");
                                                                inviaUDP(sd, answer, strlen(answer), myInfo.vicino1);
                                                        }
                                                        else {
                                                                printf("Inoltro la richiesta a %d\n", myInfo.vicino1);
                                                                inviaUDP(sd, buffer, strlen(buffer), myInfo.vicino1);
                                                        }
                                                }
                                        }
                                        else {
                                                printf("Ho la risposta che cerca il peer %d\n", req_port);
                                                ret = sprintf(answer, "%s %d", "AGGR_REP", aggr);
                                                answer[ret] = '\0';
                                                inviaUDP(sd, answer, ret, req_port);
                                        }
                                }

                                        //Messaggio di richiesta entries
                                if(strcmp(temp_buffer, "EP2P_REQ") == 0){
                                        char r_type;
                                        //Invio ack

                                        //Leggo il numero del mittente riciclando util_port
                                        sscanf(buffer, "%s %d %c", temp_buffer, &util_port, &r_type);
                                        //Invio tutte le entries mancanti
                                        inviaEntriesMancanti(util_port, r_type, "EP2P_REP", myInfo.porta, sd);

                                        //Inoltro richiesta o invio alt
                                        if(myInfo.vicino1 == util_port){
                                                printf("Invio HLT al peer %d che ha fatto partire il giro di richieste\n", myInfo.vicino1);
                                                inviaUDP(sd, "EP2P_HLT", MAX_TIPO, myInfo.vicino1);
                                        }
                                        else {
                                                printf("Inoltro la richiesta partita da %d al peer %d\n", util_port, myInfo.vicino1);
                                                inviaUDP(sd, buffer, strlen(buffer), myInfo.vicino1);
                                        }
                                }

                                //Nuova entry inviata al momento della chiusura
                                if(strcmp(temp_buffer, "EP2P_NEW") == 0){

                                        printf("Inserisco entry %s\n", buffer+9);
                                        inserisciEntryStringa(buffer+9, myInfo.porta);
                                }
                        }
                        FD_CLR(sd, &readset);
                }

        }
        return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
