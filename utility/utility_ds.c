#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

/*****************COSTANTI**************************/
#define SPAZIO " \n" //Torna utile quando devo scorrere una stringa e mi serve un delimitatore
#define BUFLEN 1024//Dimensione massima del buffer
#define LOCALHOST "127.0.0.1"//Indirizzo predefinito
#define MAX_SOCKET_RECV 630 //Dimentsione massima messaggio ricevuto
#define MAX_TIPO 15 //Dimensione massima tipo richiesta al ds
#define MAX_LISTA 21 //Dimensione massima lista vicini
#define MAX_DATA 15
#define MAX_TEMPO 8
#define MAX_FILE 31
#define MAX_SOMMA 19 //Per scelta progettuale i dati aggregati sono al massimo dell'ordine di un miliardo
#define MAX_ENTRY 16
#define TREMILASEICENTO 3600
/****************FINE COSTANTI********************/

/********************************VARIABILI*********************************/
int sd;
int peersConnessi=0;
int EntriesGiornaliere;
char buffer[BUFLEN];//buffer gestione socket
char recv_buffer[BUFLEN]; //Messaggio di richiesta connessione
struct sockaddr_in my_addr;
//Variabili per gestire input da socket oppure da stdin
fd_set readset; //set di descrittori pronti
fd_set master; // set di descrittori da monitorare
int fdmax; //Descrittore max

struct Entry {
	char date[11];
	int num_entry_N;;
	int num_entry_T;
} DS_entry;

struct Entry trovaEntry;
time_t tempoOraTemp;
struct tm* tmOraTemp;

struct tm convertiData;
time_t dataminima, datamassima, date_tmp;

struct timeval *timeout;
char dataOra[MAX_DATA+1];
char tempoOra[MAX_TEMPO+1];
int debug=0;
/*****************************FINE VARIABILI*********************************/




void pulisciIndirizzi(struct sockaddr_in* addr_p,  int port){
    memset(addr_p, 0, sizeof((*addr_p)));
	addr_p->sin_family = AF_INET;
	addr_p->sin_port = htons(port);
	inet_pton(AF_INET, LOCALHOST, &addr_p->sin_addr);
}



int creaSocketAscolto(struct sockaddr_in* my_addr,int porta) {
        int sd;
        int ret;
        sd = socket(AF_INET, SOCK_DGRAM, 0);// Creazione socket UDP
        pulisciIndirizzi(my_addr,porta);
        ret = bind(sd, (struct sockaddr*)my_addr, sizeof(*my_addr));/*Aggancio*/
        if( ret < 0 ){
                perror("Bind non riuscita\n");
                exit(0);
        }
        printf("Discovery server in ascolto sulla porta:%d\n\n", porta);
        return sd;
}

int riceviUDP(int socket, char* buffer, int buff_l){
        int ret;
        struct sockaddr_in send_addr;
        socklen_t send_addr_len;
        send_addr_len = sizeof(send_addr);
        do {
		ret = recvfrom(socket, buffer, buff_l, 0, (struct sockaddr*)&send_addr, &send_addr_len);
	} while (ret < 0);
        return ntohs(send_addr.sin_port);
}

void inviaUDP(int socket, char* buffer, int buff_l, int recv_port){
        int ret;
        struct sockaddr_in recv_addr;
        socklen_t recv_addr_len;
        pulisciIndirizzi(&recv_addr, recv_port);
        recv_addr_len=sizeof(recv_addr);
        ret = 0;
        do {
                ret = sendto(socket, buffer, buff_l+1, 0, (struct sockaddr*)&recv_addr, recv_addr_len);
        } while(ret<0);

        printf("Messaggio %s inviato correttamente al destinatario %d\n", buffer, recv_port);
}







/***************** GESTORE FILE********************/
int alreadyBooted(int porta){
    FILE *fd;
    char temp_buffer[INET_ADDRSTRLEN];
    int temp;

    fd = fopen("./txtDS/bootedPeers.txt", "r");

    if(fd){
        while(fscanf(fd, "%s %d", temp_buffer, &temp)==2){
            if(temp == porta)
                return 1;
        }
    }

    return 0;
}

int inserisci_peer(char* addr, int port, int peersConnessi){
        FILE *fp, *temp;
        int m, f, servo;
        int temp_port[2];
        char temp_buff[INET_ADDRSTRLEN];

        //Se primo peer a connettersi
        if(peersConnessi == 0){
                fp = fopen("./txtDS/bootedPeers.txt", "w");
                fprintf(fp, "%s %d\n", addr, port);
                fclose(fp);
                return 1;
        }

        //Inserisco ordinatamente nel caso di un peer gia' presente
        if(peersConnessi == 1){

                temp_port[1] = -1;//non utilizzo la porta 1
                fp = fopen("./txtDS/bootedPeers.txt", "r");
                temp = fopen("temp.txt", "w");

                fscanf(fp, "%s %d", temp_buff, &temp_port[0]);
                if(temp_port[0] < port){
                        fprintf(temp, "%s %d\n", temp_buff, temp_port[0]);
                        fprintf(temp, "%s %d\n", addr, port);
                }
                else {
                        fprintf(temp, "%s %d\n", addr, port);
                        fprintf(temp, "%s %d\n", temp_buff, temp_port[0]);
                }
                fclose(temp);
                fclose(fp);
                remove("./txtDS/bootedPeers.txt");
                rename("temp.txt", "./txtDS/bootedPeers.txt");

                return 1;
        }

        //Inserisco ordinatamente nel caso di due o piu' peer gia' presenti
        else {
                fp = fopen("./txtDS/bootedPeers.txt", "r");
                temp = fopen("temp.txt", "w");

                temp_port[0] = -1;
                //Prelevo i primi valori
                m = fscanf(fp, "%s %d", temp_buff, &servo);
                //Salvo il primo numero
                f = servo;
                //Finche' trovo valori e non arrivo alla fine...
                while(servo < port && m == 2){
                        //Salvo il precedente
                        temp_port[0] = servo;
                        //Lo stampo
                        fprintf(temp, "%s %d\n", temp_buff, servo);
                        //Provo a prenderne un altro
                        m = fscanf(fp, "%s %d", temp_buff, &servo);
                        //Lo metto come successivo
                        temp_port[1] = servo;
                }

                //Quando esco dal while inserisco il valore
                fprintf(temp, "%s %d\n", temp_buff, port);

                //Se non sono mai entrato nel while oppure sono arrivato in fondo
                //ovvero il mio valore e' il minimo o il massimo
                if(servo == f || m != 2)
                        //Il peer successivo e' il primo che ho estratto
                        temp_port[1] = f;

                //Se non ero arrivato in fondo al file
                if(m == 2)
                        //Stampo il valore successivo a quello che dovevo inserire
                        fprintf(temp, "%s %d\n", temp_buff, servo);

                //Inserisco gli altri finche' non finiscono
                while(fscanf(fp, "%s %d", temp_buff, &servo) == 2)
                        fprintf(temp, "%s %d\n", temp_buff, servo);

                //Se non ero mai entrato nel while
                if(temp_port[0] == -1)
                        //Il mio peer precedente e' il massimo, perche' io sono il minimo
                        temp_port[0] = servo;

                //Fine
                fclose(temp);
                fclose(fp);
                remove("./txtDS/bootedPeers.txt");
                rename("temp.txt", "./txtDS/bootedPeers.txt");

                return 1;
        }

        return -1;
}

void rimuoviPeer(int port){
        FILE *fd, *temp;
        char temp_buffer[INET_ADDRSTRLEN];
        int servo,ret;

        fd = fopen("./txtDS/bootedPeers.txt", "r");

        ret = fscanf(fd, "%s %d", temp_buffer, &servo);
        if(ret < 2){
                fclose(fd);
                remove("./txtDS/bootedPeers.txt");
        }
        else {
                temp = fopen("temp.txt", "w");

        while(servo < port){
                fprintf(temp, "%s %d\n", temp_buffer, servo);
                fscanf(fd, "%s %d", temp_buffer, &servo);
        }

        while(fscanf(fd, "%s %d", temp_buffer, &servo)==2)
                fprintf(temp, "%s %d\n", temp_buffer, servo);

        }

        fclose(fd);
        fclose(temp);
        remove("./txtDS/bootedPeers.txt");
        rename("temp.txt", "./txtDS/bootedPeers.txt");
}

//Trova il numero di porta di un peer, data la sua posizione
int trovaPorta(int posizione){ //Restituisce data la posizione il numero della porta richiesta
    FILE *fp;
    int port;
    char temp[INET_ADDRSTRLEN];
    int ret;

    fp = fopen("./txtDS/bootedPeers.txt", "r");
    if(fp == NULL)
        return -1;

    while(posizione-- >= 0){
        ret = fscanf(fp, "%s %d", temp, &port);

        if(ret == EOF)
            return -1;
    }

    return port;
}


void trovaVicini(int peer, int peersConnessi, int *vicino1, int *vicino2){ //Trova i vicini di un peer e li restituisce
        FILE *fd;
        int m,first,temp;
        char temp_buff[INET_ADDRSTRLEN];

        fd = fopen("./txtDS/bootedPeers.txt", "r");
        //Algoritmo simile a quello di inserimento

        (*vicino1) = -1;
        (*vicino2) = -1;

        //Se un solo peer connesso
        if(peersConnessi == 1)
                return;

        //Se ce ne sono due
        if(peersConnessi == 2){
                (*vicino1) = (trovaPorta(0) == peer) ? trovaPorta(1) : trovaPorta(0);
                return;
        }

        //Prelevo i primi valori
        m = fscanf(fd, "%s %d", temp_buff, &temp);
        //Salvo il primo numero
        first = temp;
        //Finche' trovo valori e non arrivo alla fine...
        while(temp < peer && m == 2){
                (*vicino1) = temp;//Salvo il precedente
                m = fscanf(fd, "%s %d", temp_buff, &temp);//Provo a prenderne un altro
                //Se ho trovato il peer di cui devo cercare i vicini metto come successivo il successivo o il primo, nel caso il mio peer sia il massimo
                if(temp == peer){
                        if(fscanf(fd, "%s %d", temp_buff, &temp) == 2)//Provo a estrarre un altro peer
                                (*vicino2) = temp;//Lo metto come successivo
                        else//Altrimenti metto il primo (il peer in esame e' il massimo
                                (*vicino2) = first;
                }
        }


        if((*vicino1) == -1 && (*vicino2) == -1){ //Se non ero mai entrato nel while
                m = fscanf(fd, "%s %d", temp_buff, &temp); //Il successivo e' il secondo della lista
                (*vicino2) = temp; //Salvo il primo numero
                while(fscanf(fd, "%s %d", temp_buff, &temp) == 2) //Il precedente e' l'ultimo della lista
                        (*vicino1) = temp;
        }
}

void trovaLista(int peer, int peersConnessi, char* mess_type, char* list_buffer, int* list_length){
        int temp_port[2];
        trovaVicini(peer, peersConnessi, &temp_port[0], &temp_port[1]);
        printf("Vicini di %d: %d e %d\n", peer, temp_port[0], temp_port[1]);
        //Compongo la lista
        if(temp_port[0] == -1 && temp_port[1] == -1)
                (*list_length) = sprintf(list_buffer, "%s", mess_type);
        else if(temp_port[1] == -1)
                (*list_length) = sprintf(list_buffer, "%s %d", mess_type, temp_port[0]);
        else
                (*list_length) = sprintf(list_buffer, "%s %d %d", mess_type, temp_port[0], temp_port[1]);

        list_buffer[(*list_length)] = '\0';
        printf("Lista preparata: %s\n", list_buffer);
}

void trovaTempo(){

    tempoOraTemp = time(NULL);
    tmOraTemp = gmtime(&tempoOraTemp);

    if(tmOraTemp->tm_hour < 18)
        sprintf(dataOra, "%04d:%02d:%02d", tmOraTemp->tm_year+1900, tmOraTemp->tm_mon+1, tmOraTemp->tm_mday);
    else {
        tmOraTemp->tm_mday += 1;
        tempoOraTemp = mktime(tmOraTemp);
        tmOraTemp = gmtime(&tempoOraTemp);
        sprintf(dataOra, "%04d:%02d:%02d", tmOraTemp->tm_year+1900, tmOraTemp->tm_mon+1, tmOraTemp->tm_mday);
    }
    dataOra[MAX_DATA] = '\0';
    sprintf(tempoOra, "%02d:%02d:%02d", tmOraTemp->tm_hour, tmOraTemp->tm_min, tmOraTemp->tm_sec);
    tempoOra[MAX_TEMPO] = '\0';
}



void inserisciEntry(){
        FILE *fd1;
        char filename[MAX_FILE];
        printf("Sono dentro la Entry\n" );
        trovaTempo();
        sprintf(filename, "%s%s", "./txtDS/", "entries.txt");

        fd1 = fopen(filename, "a");
        if(fd1 == NULL){
                printf("Errore, impossibile aprire il file entries\n\n>");
                return;
        }
        else {
                printf("Entry Inserita\n\n>" );
                trovaTempo();
                strftime(DS_entry.date, sizeof(DS_entry.date), "%d_%m_%Y", tmOraTemp);
                fprintf(fd1, "%s %i %i\n", DS_entry.date, DS_entry.num_entry_N, DS_entry.num_entry_T);
                DS_entry.num_entry_N=0;
                DS_entry.num_entry_T=0;
        }
        fclose(fd1);
}


int leggiEntries(int tipo, char bound1[MAX_DATA], char bound2[MAX_DATA], int portaPeer){
        FILE *fd;
        char filename[MAX_FILE];
        int totaleEntriesPeriodo=0;
        char entr_repl[MAX_ENTRY];
        int ret;
        char bound1_temp[MAX_DATA];
        char bound2_temp[MAX_DATA];

        convertiData.tm_hour = convertiData.tm_min = convertiData.tm_sec = 0;

        int appoggioData[2][3];

        if(strcmp(bound1, "*") != 0) {
                sscanf(bound1, "%d:%d:%d", &appoggioData[0][2], &appoggioData[0][1], &appoggioData[0][0]);
                sprintf(bound1_temp, "%i_%i_%i",appoggioData[0][2], appoggioData[0][1], appoggioData[0][0]);
                strptime(bound1_temp, "%d_%m_%Y", &convertiData);
                dataminima = mktime(&convertiData)+3600;


        }

        if(strcmp(bound2, "*") != 0) {
                sscanf(bound2, "%d:%d:%d", &appoggioData[1][2], &appoggioData[1][1], &appoggioData[1][0]);
                sprintf(bound2_temp, "%i_%i_%i", appoggioData[1][2], appoggioData[1][1], appoggioData[1][0]);
                strptime(bound2_temp, "%d_%m_%Y", &convertiData);
                datamassima = mktime(&convertiData)+3600;


        }

        sprintf(filename, "%s%s", "./txtDS/", "entries.txt");
        fd = fopen(filename, "r");
        if(fd == NULL)
            return 0;
        while(fscanf(fd, "%s %i %i\n", trovaEntry.date, &trovaEntry.num_entry_N, &trovaEntry.num_entry_T) != EOF) {
        //conversione data prelevata

                strptime(trovaEntry.date, "%d_%m_%Y", &convertiData);
                date_tmp = mktime(&convertiData)+3600;
                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") != 0){
                                if((difftime(dataminima, date_tmp) <= 0) && (difftime(datamassima, date_tmp) >= 0)){
                                        if(!tipo)
                                                totaleEntriesPeriodo+=trovaEntry.num_entry_N;
                                        else
                                                totaleEntriesPeriodo+=trovaEntry.num_entry_T;

                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") != 0){
                                if(difftime(datamassima, date_tmp) >= 0){
                                        if(!tipo)
                                                totaleEntriesPeriodo+=trovaEntry.num_entry_N;
                                        else
                                                totaleEntriesPeriodo+=trovaEntry.num_entry_T;
                                }
                        }
                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") == 0){
                                if((difftime(dataminima, date_tmp) <= 0)){
                                        if(!tipo)
                                                totaleEntriesPeriodo+=trovaEntry.num_entry_N;
                                        else
                                                totaleEntriesPeriodo+=trovaEntry.num_entry_T;
                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") == 0){
                                if(!tipo)
                                        totaleEntriesPeriodo+=trovaEntry.num_entry_N;
                                else
                                        totaleEntriesPeriodo+=trovaEntry.num_entry_T;

                        }

        }
        ret = sprintf(entr_repl, "%s %i", "ENTR_REP", totaleEntriesPeriodo);
        entr_repl[ret] = '\0';
        inviaUDP(sd, entr_repl, ret, portaPeer);
        printf("\n\n>");
        return 1;
}


void stampaPeers(int peersConnessi){
    int i;
    printf("Peer connessi alla rete:");
    if(!peersConnessi)
        printf(" nessuno!");

    for(i=0; i<peersConnessi; i++)
        printf("\n%d", trovaPorta(i));

    printf("\n");
}

void stampaVicino(int peersConnessi, int porta){
    if(!alreadyBooted(porta))
        printf("Peer %d non connesso alla rete!\n", porta);
    else {
        int temp_port[2];
        trovaVicini(porta, peersConnessi, &temp_port[0], &temp_port[1]);
        printf("Vicini del peer %d: ", porta);
        if(temp_port[0] == -1 && temp_port[1] == -1)
            printf("nessuno!\n");
        else if(temp_port[1] == -1)
            printf("soltanto %d\n", temp_port[0]);
        else
            printf("%d e %d\n", temp_port[0], temp_port[1]);
    }
}

void stampaTuttiVicini(int peersConnessi){
    int i;
    switch(peersConnessi){
        case 1:
            printf("Vicini del peer %d: nessuno!\n", trovaPorta(0));
            break;
        case 2:
            printf("Vicini del peer %d: soltanto %d\n", trovaPorta(0), trovaPorta(1));
            printf("Vicini del peer %d: soltanto %d\n", trovaPorta(1), trovaPorta(0));
            break;
        default:
            for(i=0; i<peersConnessi; i++)
                printf("Vicini del peer %d: %d e %d\n", trovaPorta(i), trovaPorta((i-1+peersConnessi)%peersConnessi), trovaPorta((i+1)%peersConnessi));
    }
}
/***************** FINE GESTORE FILE********************/


//Valuta Chiusura File Odietrno

void checkTime() {	//controlla se bisogna chiudere il file odierno
        struct tm* tmOraTemp;
        time_t tempoOraTemp;

        tempoOraTemp = time(NULL);
        tmOraTemp = gmtime(&tempoOraTemp);
        if(((tmOraTemp->tm_hour == 18 && tmOraTemp->tm_min==1) || debug==1 ) ){ // Aspetto un minuto per ricevere tutte le Entries
                //sleep(5);
                inserisciEntry();
		printf("Registro della data odierna chiuso\n");
        }

}
