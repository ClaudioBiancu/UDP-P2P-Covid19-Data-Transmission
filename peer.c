#include "./utility/utility_peer.c"


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

        printf("Formato comando Corretto\n");

        if(tipo=='N'){
                Peer_entry.num_entry_N=quanto;
                NumeroEntryN++;
                inserisciEntry(0, myInfo.porta);
        }
        else{
                NumeroEntryT++;
                Peer_entry.num_entry_T=quanto;
                inserisciEntry(1, myInfo.porta);
        }




}

void get(char*parola){
        char aggr;
        int tipo;
        char bound[2][MAX_DATA+1];
        int ret=3; //Inizialmente lo uso per gestire il numero dei parametri, che minimo sono 3
        int tot_entr;
        int peer_entr;
        int sum_entr;
        int temp_buffer[200];
        char get_buffer[200]; //Ricevere il messaggio
        char util_buffer[200];
        char date_buffer[200];
        char altro_buffer[MAX_SOMMA];
        int r_aggr;
        int r_type;
        int r_quantity;
        char r_bound1[18];
        char r_bound2[18];
        char entry_ins_buffer[MAX_PEERS*6];
        char control_buffer[7];
        int r_port;
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
                        aggr='t';
                else
                        aggr='v';
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
                        tipo=0;
                else
                        tipo=1;
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
        }
        else{ //L'utente non ha inserito un periodo
                strcpy(bound[0], "*");
                strcpy(bound[1], "*");
                if(!controllaDate(bound[0], bound[1], aggr))
                        return;
        }

        if(controllaAggr(bound[0], bound[1], aggr, tipo)){
                //stampaRisultati();
                return;
        }

        sum_entr = 0;
        //Invio richiesta al server
        ret = sprintf(get_buffer, "%s %i %s %s", "ENTR_REQ", tipo, bound[0], bound[1]);
        inviaUDP(sd, get_buffer, ret, portaServer);

        //Ricevo risposta
        riceviUDP(sd, get_buffer, MAX_ENTRY);

        sscanf(get_buffer, "%s %i", temp_buffer, &tot_entr);

        peer_entr = contaEntries(tipo, bound[0], bound[1]);

        printf("Entries nel server: %d; entries qui: %d\n", tot_entr, peer_entr);

        //Se nessuna entry, inutile continuare
        if(!tot_entr)
                printf("Nessun dato aggregato da calcolare\n");
        //Se ho tutti i dati che servono, eseguo il calcolo e lo scrivo
        if((tot_entr == peer_entr ) && tot_entr){
                scriviAggr(bound[0], bound[1], aggr, tipo, 0);
                stampaRisultati();
                return;
        }
        //Altrimenti
        else {
                //Se non ho vicini non posso calcolare nulla
                if(myInfo.vicino1 == -1 && myInfo.vicino2 == -1)
                        printf("Non ho vicini collegati, non posso risolvere in questo momento\n");

                else { if(aggr!='v'){
                                printf("Devo chiedere informazioni ai miei vicini\n");
                                //Controllo se qualche peer ha il dato aggregato pronto
                                sprintf(get_buffer, "%s %i %s %s", "ENTR_REQ", tipo, bound[0], bound[1]);
                                ret = sprintf(get_buffer, "%s %i %i %c %s %s", "REQ_DATA", myInfo.porta, tipo, aggr, bound[0], bound[1]);
                                get_buffer[ret] = '\0';

                                inviaUDP(sd, get_buffer, ret, myInfo.vicino1);

                                riceviUDP(sd, get_buffer, MAX_SOMMA);

                                sscanf(get_buffer, "%s %i", temp_buffer, &sum_entr);
                        }
                        if(sum_entr >= 0){
                                printf("Ho ottenuto il dato che cercavo\n");
                                sendAggregato.totale=sum_entr;
                                scriviAggr(bound[0], bound[1], aggr, tipo, 1);
                        }
                        else{
                                //Necessario inviare a tutti la richiesta di nuove entries
                                ret = sprintf(get_buffer, "%s %i %i %s %s", "FLOODREQ", myInfo.porta, tipo, bound[0], bound[1]);
                                get_buffer[ret] = '\0';
                                printf("\nNessuno ha l'aggregato, provo il Flood:\n");
                                inviaUDP(sd, get_buffer, ret, myInfo.vicino1);

                                //Aspetto tutte le entries che mi mancano
                                while(peer_entr < tot_entr){
                                        //Nuove entries: possono arrivare da tutti
                                        ret = (myInfo.vicino1 != -1) ? 1 : 0;
                                        riceviUDP(sd, altro_buffer, MAX_UPDATEENTRY);
                                        sscanf(altro_buffer,  "%s %i %s %i %i", util_buffer, &r_type, date_buffer, &r_quantity, &r_port);
                                        if(r_type==-1)
                                                continue;
                                        ret = sprintf(altro_buffer, "%s %i %i %i", date_buffer, r_type, r_quantity, r_port);
                                        altro_buffer[ret] = 0;
                                        if(!entryPresente(altro_buffer)){
                                                Peer_entry.num_entry_N=r_quantity;
                                                strcpy(Peer_entry.date, date_buffer);
                                                inserisciEntry(r_type, r_port);
                                                peer_entr++;
                                                if (peer_entr < tot_entr){
                                                        ret = sprintf(get_buffer, "%s %i", "FLOODSTP", 1);
                                                        inviaUDP(sd, get_buffer, ret, r_port);
                                                }
                                        }
                                }
                                if(r_type==-1){
                                        printf("Errore nel ricezione delle Entry, fine corsa\n");
                                }



                                printf("Tutte le entries necessarie sono arrivate a destinazione\n\n");
                                //Eseguo il calcolo
                                //sum_entr = sommaEntries(tipo, myInfo.porta);
                                //Lo riporto sul file
                                scriviAggr(bound[0], bound[1], aggr, tipo, 0);
                        }
                }
        }

}

void stop(){
        //Controllo che la connessione esista
        if(portaServer== -1)
            printf("Il peer non e' connesso al DS. Uscita\n");

        else{
                inviaEntryDS();
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

                timeout = malloc(sizeof(timeout));
                timeout->tv_sec = 15;
                timeout->tv_usec = 0;

                select(fdmax+1, &readset, NULL, NULL, timeout);

                if(portaServer != -1)
			checkTime();

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
                                if(strcmp(temp_buffer, "REQ_DATA") == 0){
                                        int req_port;
                                        int tipo;
                                        char aggr;
                                        char answer[MAX_ENTRY];
                                        char bound1[MAX_DATA];
                                        char bound2[MAX_DATA];
                                        int aggrPosseduto;


                                        //Leggo il messaggio
                                        sscanf(buffer, "%s %i %i %c %s %s", temp_buffer, &req_port, &tipo, &aggr, bound1, bound2);

                                        aggrPosseduto = controllaAggr(bound1, bound2, aggr, tipo);

                                        if(aggrPosseduto == 0){
                                                if(myInfo.vicino2 == -1){
                                                        if(myInfo.vicino1 != req_port)
                                                                printf("Errore insolubile, crash della rete\n");
                                                        else {
                                                                printf("Invio direttamente la risposta negativa a %d\n", myInfo.vicino1);
                                                                ret = sprintf(answer, "%s %i ", "REP_DATA", -1);
                                                                inviaUDP(sd, answer, strlen(answer), myInfo.vicino1);
                                                        }
                                                }
                                                else {
                                                        if(myInfo.vicino1 == req_port){
                                                                printf("Invio risposta negativa a %d", myInfo.vicino1);
                                                                ret = sprintf(answer, "%s %i ", "REP_DATA", -1);
                                                                inviaUDP(sd, answer, strlen(answer), myInfo.vicino1);
                                                        }
                                                        else {
                                                                printf("Inoltro la richiesta a %d\n", myInfo.vicino1);
                                                                inviaUDP(sd, buffer, strlen(buffer), myInfo.vicino1);
                                                        }
                                                }
                                        }
                                        else {
                                                aggrPosseduto=sendAggregato.totale;
                                                ret = sprintf(answer, "%s %i ", "REP_DATA", aggrPosseduto);
                                                answer[ret] = '\0';
                                                inviaUDP(sd, answer, ret, req_port);
                                        }
                                }

                                        //Messaggio di richiesta entries
                                if(strcmp(temp_buffer, "FLOODREQ") == 0){
                                        char r_type;
                                        int req_port;
                                        int tipo;
                                        char bound1[MAX_DATA];
                                        char bound2[MAX_DATA];
                                        char bufferErr[MAX_LISTA];
                                        int risultato;

                                        //Leggo il numero del mittente riciclando util_port
                                        sscanf(buffer, "%s %i %i %s %s", temp_buffer, &req_port, &tipo, bound1, bound2);
                                        //Invio tutte le entries mancanti
                                        risultato=inviaEntriesMancanti(req_port, tipo, bound1, bound2, "FLOODREP");

                                        //Inoltro richiesta o invio alt
                                        if(!risultato){
                                                if(myInfo.vicino1==req_port|| myInfo.vicino1==-1 ){
                                                        if(!miDevoFermare){
                                                                ret = sprintf(bufferErr, "%s %i %s  %i %i", "FLOODREP",-1, "fineCorsa", 0, myInfo.porta);
                                                                inviaUDP(sd, bufferErr, strlen(buffer), req_port);
                                                                miDevoFermare=0;
                                                        }
                                                }
                                                else{
                                                        if(!miDevoFermare){
                                                                inviaUDP(sd, buffer, strlen(buffer), myInfo.vicino1);
                                                        }
                                                }
                                        }
                                        miDevoFermare=0;
                                }
                        }
                        if(strcmp(temp_buffer, "FLOODSTP") == 0){
                                printf("%s\n","Il richiedente ha tutte le entries, mi devo fermare");
                                miDevoFermare=1;

                        }

                        FD_CLR(sd, &readset);
                }

        }
        return 0;
}

/***************************************FINE SEZIONE MAIN*********************************/
