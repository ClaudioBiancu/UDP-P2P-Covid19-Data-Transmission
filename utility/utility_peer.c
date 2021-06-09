//File contenente costanti e funzioni di utilita' per i peer
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
//#define _XOPEN_SOURCE
#include <time.h>
/*****************COSTANTI**************************/
//Torna utile quando devo scorrere una stringa e mi serve un delimitatore
#define SPAZIO " \n"
//Dimensione massima del buffer
#define BUFLEN 1024
#define LOCALHOST "127.0.0.1"
#define MAX_SOCKET_RECV 630 //Dimentsione massima messaggio ricevuto
#define MAX_TIPO 8 //Dimensione massima tipo richiesta al ds
#define MAX_LISTA 21 //Dimensione massima lista vicini
#define TIPO_ENTRY 10
#define MAX_DATA 15
#define MAX_TEMPO 8
#define MAX_FILE 31
#define ATTESA_BOOT 5
#define MAX_SOMMA 19
#define MAX_PEERS 100
#define MAX_ENTRY 30
#define MAX_UPDATEENTRY 630
#define TZ  3600
#define MAX_ARRAY 365 //scelta progettuale variazione su massimo un anno




/***************** FINE COSTANTI********************/

/********************************VARIABILI*********************************/
//socket di ascolto del peer
int sd;//socket di ascolto del peer
int ret; //variabile di servizio
char buffer[BUFLEN]; //buffer utilizzato dal socket
struct sockaddr_in socketAscolto_addr; // Struttura per gestire il socket di ascolto del peer
int portaServer;

//struttura che descrive il mio peer

//Variabili per gestire input da socket oppure da stdin
fd_set readset; //set di descrittori pronti
fd_set master; // set di descrittori da monitorare
int fdmax; //Descrittore max


struct Entry {
	char date[11];
	int num_entry_N;;
	int num_entry_T;
} Peer_entry;

struct EntrySomma{
        char data[MAX_DATA];
        int tipo;
        int quanti;
} sommaVar;
struct EntrySomma ArraySomma[MAX_ARRAY];
struct EntrySomma ArrayVariazione[MAX_ARRAY];

struct Peer{
        int porta;
        int vicino1;
        int vicino2;
} myInfo;

struct Aggregato{
        char bound1[11];
        char bound2[11];
        char aggr;
        int tipo;
        int totale;
} sendAggregato;

int NumeroEntryN=0;
int NumeroEntryT=0;

time_t tempoOraTemp;
struct tm* tmOraTemp;
int miDevoFermare=0;

struct tm dateToConvert;
time_t min_date_given, max_date_given, date_tmp, ultimaData_temp;
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

//Crea il socket UDP collegandosi al server con indirizzo passato come primo parametro
//e alla porta passata come secondo parametro, restituisce un descrittore di socket
int creaSocketAscolto(struct sockaddr_in* socket_ascolto, int porta) {
        int ret;
        int sd;
        // Creazione socket
        sd = socket(AF_INET, SOCK_DGRAM, 0);// Creazione socket UDP
        pulisciIndirizzi(socket_ascolto, porta);
        ret = bind(sd, (struct sockaddr*)socket_ascolto, sizeof(*socket_ascolto));
        if(ret<0){
                perror("Errore in fase di binding");
                exit(0);
        }
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
        printf("Messaggio ricevuto: %s\n", buffer);
        return ntohs(send_addr.sin_port);
}

void inviaUDP(int socket, char* buff, int buff_l, int recv_port){
        int ret=-1;
        struct sockaddr_in recv_addr;
        socklen_t recv_addr_len;
        pulisciIndirizzi(&recv_addr, recv_port);
        recv_addr_len=sizeof(recv_addr);
        //Invio lista
        do{
                ret = sendto(socket, buff, buff_l+1, 0, (struct sockaddr*)&recv_addr, recv_addr_len);

        }while(ret<0);

        printf("Messaggio %s inviato correttamente al destinatario %d\n", buff, recv_port);
}





void inviaEntryDS() {
        char nuovaEntry[TIPO_ENTRY+1];
        printf("invio Entries %i %i\n", NumeroEntryN, NumeroEntryT);
        ret = sprintf(nuovaEntry, "%s %i %i", "NEW_ENTR", NumeroEntryN, NumeroEntryT);
        nuovaEntry[ret] = '\0';
        inviaUDP(sd, nuovaEntry, ret, portaServer);
        NumeroEntryT=0;
        NumeroEntryN=0;
}

//Recupera la data e l'ora correnti e le inserisce nelle variabili globali lato client
void trovaTempo(){

        tempoOraTemp = time(NULL);
        tmOraTemp = gmtime(&tempoOraTemp);
        sprintf(dataOra, "%04d:%02d:%02d", tmOraTemp->tm_year+1900, tmOraTemp->tm_mon+1, tmOraTemp->tm_mday);
        dataOra[MAX_DATA] = '\0';
        sprintf(tempoOra, "%02d:%02d:%02d", tmOraTemp->tm_hour, tmOraTemp->tm_min, tmOraTemp->tm_sec);
        tempoOra[MAX_TEMPO] = '\0';
}

int periodoValido(int *date1, int *date2, char aggr){
        return (date1[0] < date2[0] || (date1[0] == date2[0] && (date1[1] < date2[1] || (date1[1] == date2[1] && (date1[2] < date2[2] || (date1[2] == date2[2] && aggr == 't'))))));
}
//Controllo la validita' della data
int dataValida(int y, int m, int d){
        int c_date[3];
        int this_date[3];
        this_date[0] = y;
        this_date[1] = m;
        this_date[2] = d;
        //Basic
        if(y < 2021 || m < 1 || m > 12 || d < 1 || d > 31){
                printf("E1\n");
                return 0;
        }
        trovaTempo();
        sscanf(dataOra, "%d:%d:%d", &c_date[0], &c_date[1], &c_date[2]);
        //Sfrutto la funzione is_real_period
        if(!periodoValido(this_date, c_date, 't')){
                printf("E2\n");
                return 0;
        }
        //Advanced
        if(m == 4 || m == 6 || m == 9 || m == 11)
        return (d <= 30);
        //Extreme
        if(m == 2){
        if(((y%4 == 0) && (y%100 != 0)) || (y%400 == 0))
                return (d <= 29);
        else
                return (d <= 28);
        }

        return 1;
}

int oggi(char* bound_date){
        int b_date[3];
        int c_date[3];

        trovaTempo();
        sscanf(bound_date, "%d:%d:%d", &b_date[2], &b_date[1], &b_date[0]);
        sscanf(dataOra, "%d:%d:%d", &c_date[0], &c_date[1], &c_date[2]);

        return (b_date[0] == c_date[0] && b_date[1] == c_date[1] && b_date[2] == c_date[2]);
}

//Controllo che le date input della get siano valide
int controllaDate(char *date1, char *date2, char aggr){
        int date[2][3];
        int ret[2];
        //Controllo sulla prima data
        //Conversione da dd:mm:yyyy a yyyy:mm:dd
        ret[0] = sscanf(date1, "%d:%d:%d", &date[0][2], &date[0][1], &date[0][0]);
        if(!(ret[0] == 3 || strcmp(date1, "*") == 0)){
                printf("Data 1 non conforme al tipo di input richiesto");
                return 0;
        }
        if(ret[0] == 3 && !dataValida(date[0][0], date[0][1], date[0][2])){
                printf("Errore nell'inserimento della prima data\n");
                return 0;
        }
        //Controllo sulla seconda data
        //Conversione da dd:mm:yyyy a yyyy:mm:dd
        ret[1] = sscanf(date2, "%d:%d:%d", &date[1][2], &date[1][1], &date[1][0]);
        if(!(ret[1] == 3 || strcmp(date2, "*") == 0)){
                printf("Data 2 non conforme al tipo di input richiesto");
                return 0;
        }
        if(ret[1] == 3 && !dataValida(date[1][0], date[1][1], date[1][2])){
                printf("Errore nell'inserimento della seconda data\n");
                return 0;
        }
        //Se entrambe le date sono * non va bene

        /*Se entrambe le date sono presenti,
        controllo che la seconda non sia minore della prima in caso di totale
        e sia strettamente maggiore in caso di variazione*/
        if(ret[0] == 3 && ret[1] == 3){
                if(!periodoValido(date[0], date[1], aggr)){
                        printf("Periodo scelto non coerente\n");
                return 0;
                }
        }
        //Se una data corrisponde a oggi e il register non e' ancora chiuso, cioe' non sono ancora le 18, non si potra' eseguire la get



        //Tutto ok
        return 1;
}




/***************** GESTORE FILE********************/

void inserisciEntry(int tipo, int portaNuova){

        FILE *fd1;
        char filename[MAX_FILE];

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", "entries", myInfo.porta);

        fd1 = fopen(filename, "a");
        if(fd1 == NULL){
                printf("Errore, impossibile aprire il file entries");
                return;
        }
        else {
                trovaTempo();
                if(portaNuova==myInfo.porta)
                        strftime(Peer_entry.date, sizeof(Peer_entry.date), "%d_%m_%Y", tmOraTemp);
                if(tipo){
                        fprintf(fd1, "%s %i %i %i \n", Peer_entry.date, tipo, Peer_entry.num_entry_T, portaNuova);
                }
                else{
                        fprintf(fd1, "%s %i %i %i\n", Peer_entry.date, tipo, Peer_entry.num_entry_N, portaNuova);
                }
                printf("Entry inserita!\n\n");
        }

        fclose(fd1);
}


int contaEntries(int tipo, char bound1 [MAX_DATA], char bound2[MAX_DATA]){
        FILE *fd;
        char filename[MAX_FILE];
        int porta;
        int totaleEntriesPeriodo=0;
        int quanti=1;
        int tipo_temp=-1;
        char date[11];
        char bound1_temp[MAX_DATA];
        char bound2_temp[MAX_DATA];

        dateToConvert.tm_hour = dateToConvert.tm_min = dateToConvert.tm_sec = 0;

        int appoggioData[2][3];
        //Controllo sulla prima data


        if(strcmp(bound1, "*") != 0) {
                sscanf(bound1, "%d:%d:%d", &appoggioData[0][2], &appoggioData[0][1], &appoggioData[0][0]);
                sprintf(bound1_temp, "%i_%i_%i",appoggioData[0][2], appoggioData[0][1], appoggioData[0][0]);
                strptime(bound1_temp, "%d_%m_%Y", &dateToConvert);
                min_date_given = mktime(&dateToConvert)+TZ;\
        }

        if(strcmp(bound2, "*") != 0) {
                sscanf(bound2, "%d:%d:%d", &appoggioData[1][2], &appoggioData[1][1], &appoggioData[1][0]);
                sprintf(bound2_temp, "%i_%i_%i", appoggioData[1][2], appoggioData[1][1], appoggioData[1][0]);
                strptime(bound2_temp, "%d_%m_%Y", &dateToConvert);
                max_date_given = mktime(&dateToConvert)+TZ;

        }


        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", "entries", myInfo.porta);
        fd = fopen(filename, "r");
        if(fd == NULL)
            return 0;

        while(fscanf(fd, "%s %i %i %i\n", date, &tipo_temp, &quanti, &porta) != EOF) {
        //conversione data prelevata
                strptime(date, "%d_%m_%Y", &dateToConvert);
                date_tmp = mktime(&dateToConvert)+TZ;

                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") != 0){
                                if((difftime(min_date_given, date_tmp) <= 0) && (difftime(max_date_given, date_tmp) > 0)){
                                        if(tipo==tipo_temp){
                                                totaleEntriesPeriodo++;
                                        }

                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") != 0){
                                if(difftime(max_date_given, date_tmp) > 0){
                                        if(tipo==tipo_temp)
                                                totaleEntriesPeriodo++;
                                }
                        }
                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") == 0){
                                if((difftime(min_date_given, date_tmp) <= 0)){
                                        if(tipo==tipo_temp)
                                                totaleEntriesPeriodo++;
                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") == 0){
                                if(tipo==tipo_temp)
                                        totaleEntriesPeriodo++;

                        }

        }
        fclose(fd);
        return totaleEntriesPeriodo;
}




time_t trasforma(char data[MAX_DATA]){
        strptime(data, "%d_%m_%Y", &dateToConvert);
        return mktime(&dateToConvert)+3600;

}
int differenzaGiorni(char bound2[MAX_DATA], time_t ultima){
        float giorni;
        giorni=(difftime(ultima, trasforma(bound2)));
        giorni=giorni/86400;
        return (int)(giorni-1);

}

int calcolaVariazioneTIPO(char bound1[MAX_DATA], char bound2[MAX_DATA], int tipo, char aggr){
        FILE *fd;
        char filename[MAX_FILE];
        char directory[MAX_FILE+100];
        int i=0;
        int j=0;
        int contati=0;
        char date[MAX_DATA];
        int tipo_temp;
        int quanti;
        int porta;
        char ultimaData[MAX_DATA];
        char bound1_temp[MAX_DATA];
        char bound2_temp[MAX_DATA];
        dateToConvert.tm_hour = dateToConvert.tm_min = dateToConvert.tm_sec = 0;
        float giorni;
        int giorni_tmp;
        int appoggioData[2][3];
        int misuraArray;

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", "entries", myInfo.porta);
        fd = fopen(filename, "r");
        if(fd == NULL){
                printf("%s\n", "Non sono riuscito ad aprire il file" );
                return 0;
        }



        sscanf(bound1, "%d:%d:%d", &appoggioData[0][2], &appoggioData[0][1], &appoggioData[0][0]);
        sprintf(bound1_temp, "%i_%i_%i",appoggioData[0][2], appoggioData[0][1], appoggioData[0][0]);
        strptime(bound1_temp, "%d_%m_%Y", &dateToConvert);
        min_date_given = mktime(&dateToConvert)+TZ;


        sscanf(bound2, "%d:%d:%d", &appoggioData[1][2], &appoggioData[1][1], &appoggioData[1][0]);
        sprintf(bound2_temp, "%i_%i_%i", appoggioData[1][2], appoggioData[1][1], appoggioData[1][0]);
        strptime(bound2_temp, "%d_%m_%Y", &dateToConvert);
        max_date_given = mktime(&dateToConvert)+TZ;


        while(fscanf(fd, "%s %i %i %i\n", date, &tipo_temp, &quanti, &porta) != EOF) {
                strptime(date, "%d_%m_%Y", &dateToConvert);
                date_tmp = mktime(&dateToConvert)+3600;
                if(tipo_temp==tipo && ((difftime(min_date_given, date_tmp) <= 0) && (difftime(max_date_given, date_tmp) > 0))){
                        if(i>0){//Entries sucessive possono avere la stessa data, vado a sommare i valori
                                if(strcmp(date,ArraySomma[i-1].data)==0 ){
                                        ArraySomma[i-1].quanti+=quanti;
                                }
                                else{
                                        strcpy(ArraySomma[i].data, date);
                                        ArraySomma[i].tipo=tipo_temp;
                                        ArraySomma[i].quanti=quanti;
                                        i++;
                                }
                        }else{//Caso Prima Entries
                                strcpy(ArraySomma[i].data, date);
                                ArraySomma[i].tipo=tipo_temp;
                                ArraySomma[i].quanti=quanti;
                                i++;
                        }
                }
        }

        ArraySomma[i].quanti=-1;
        fclose(fd);
        sprintf(filename, "%s%i%s%c_%i_%s_%s.txt", "./txtPeer/", myInfo.porta,"/aggr_",aggr, tipo, bound1, bound2);
        fd = fopen(filename, "a");
        if(fd == NULL){
                sprintf(directory,"%s%i" ,"./txtPeer/",myInfo.porta);
                mkdir(directory, 0777);
                fd = fopen(filename, "a");
        }

        giorni=(difftime(max_date_given, min_date_given));
        giorni=giorni/86400;

        giorni_tmp=(int)(giorni+0.5-1);// numero di giorni tra una data e l'altra;
        printf("%i\n",giorni_tmp);

        contati=i;
        i=0;
        misuraArray=giorni_tmp;
        ArrayVariazione[misuraArray];
        while(i<misuraArray){
                ArrayVariazione[i].quanti=0;
                strcpy(ArrayVariazione[i].data,"data");
                i++;
        }
        i=contati-1;
        while(i>=0){
                ArrayVariazione[-differenzaGiorni(ArraySomma[i].data, min_date_given)-1].quanti=ArraySomma[i].quanti;
                strcpy(ArrayVariazione[-differenzaGiorni(ArraySomma[i].data, min_date_given)-1].data, ArraySomma[i].data);
                i--;
        }
        i=misuraArray-1;
        printf("%s\n\n","AGGREGATO RICHIESTO:" );
        while(i>=0){
                if(strcmp(ArrayVariazione[i].data,"data")==0 && i-1>0){
                        printf("%i) Variazione: %i\n", i, (ArrayVariazione[i].quanti-ArrayVariazione[i-1].quanti));
                        fprintf(fd, "%i)Variazione: %i\n", i, (ArrayVariazione[i].quanti-ArrayVariazione[i-1].quanti));
                }
                else
                        if((i-1)>0){
                                printf("%i) Variazione al %s: %i\n", i, ArrayVariazione[i].data, ArrayVariazione[i].quanti-ArrayVariazione[i-1].quanti);
                                fprintf(fd, "%i)Variazione_%s: %i\n", i, ArrayVariazione[i].data, ArrayVariazione[i].quanti-ArrayVariazione[i-1].quanti);
                        }
                i--;
        }
        i=i+1;
        if(strcmp(ArrayVariazione[i].data,"data")==0 ){
                printf("%i) Variazione: %i\n", i,  (ArrayVariazione[i].quanti));
                fprintf(fd, "%i)Variazione: %i\n", i,  (ArrayVariazione[i].quanti));
        }
        else{
                printf("%i) Variazione al %s: %i\n",ArrayVariazione[i].data, i,  ArrayVariazione[i].quanti);
                fprintf(fd, "%i)Variazione_%s: %i\n",ArrayVariazione[i].data, i,  ArrayVariazione[i].quanti);
        }
        fclose(fd);
        return 1;
}


int calcolaTotaleTIPO(int tipo, char bound1 [MAX_DATA], char bound2[MAX_DATA]){
        FILE *fd;
        char filename[MAX_FILE];

        int porta;
        int totaleEntriesPeriodo=0;
        int quanti=0;
        int tipo_temp=-1;
        char date[11];
        char bound1_temp[MAX_DATA];
        char bound2_temp[MAX_DATA];

        dateToConvert.tm_hour = dateToConvert.tm_min = dateToConvert.tm_sec = 0;

        int appoggioData[2][3];
        //Controllo sulla prima data


        if(strcmp(bound1, "*") != 0) {
                sscanf(bound1, "%d:%d:%d", &appoggioData[0][2], &appoggioData[0][1], &appoggioData[0][0]);
                sprintf(bound1_temp, "%i_%i_%i",appoggioData[0][2], appoggioData[0][1], appoggioData[0][0]);
                strptime(bound1_temp, "%d_%m_%Y", &dateToConvert);
                min_date_given = mktime(&dateToConvert)+TZ;


        }

        if(strcmp(bound2, "*") != 0) {
                sscanf(bound2, "%d:%d:%d", &appoggioData[1][2], &appoggioData[1][1], &appoggioData[1][0]);
                sprintf(bound2_temp, "%i_%i_%i", appoggioData[1][2], appoggioData[1][1], appoggioData[1][0]);
                strptime(bound2_temp, "%d_%m_%Y", &dateToConvert);
                max_date_given = mktime(&dateToConvert)+TZ;


        }


        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", "entries", myInfo.porta);
        fd = fopen(filename, "r");
        if(fd == NULL){
                printf("%s\n", "Non sono riuscito ad aprire il file" );
                return 0;
        }

        while(fscanf(fd, "%s %i %i %i\n", date, &tipo_temp, &quanti, &porta) != EOF) {
        //conversione data prelevata
                strptime(date, "%d_%m_%Y", &dateToConvert);
                date_tmp = mktime(&dateToConvert)+3600;
                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") != 0){
                                if((difftime(min_date_given, date_tmp) <= 0) && (difftime(max_date_given, date_tmp) > 0)){
                                        if(tipo==tipo_temp){
                                                totaleEntriesPeriodo+=quanti;
                                        }
                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") != 0){
                                if(difftime(max_date_given, date_tmp) > 0){
                                        if(tipo==tipo_temp){
                                                totaleEntriesPeriodo+=quanti;
                                        }
                                }
                        }
                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") == 0){
                                if((difftime(min_date_given, date_tmp) <= 0)){
                                        if(tipo==tipo_temp){
                                                totaleEntriesPeriodo+=quanti;
                                        }
                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") == 0){
                                if(tipo==tipo_temp)
                                        totaleEntriesPeriodo+=quanti;

                        }

        }
        fclose(fd);
        return totaleEntriesPeriodo;
}



void scriviAggr(char*bound1, char*bound2, char aggr, int tipo, int modalita){
        FILE *fd;
        char filename[MAX_FILE+100];
        char directory[MAX_FILE+100];
        int totale;
        if(aggr=='v'){
                totale= calcolaVariazioneTIPO(bound1, bound2, tipo, aggr );
        }
        else{
                totale= calcolaTotaleTIPO(tipo, bound1, bound2);


                trovaTempo();
                if(strcmp(bound2,"*")==0){
                        strftime(bound2, sizeof(Peer_entry.date), "%d:%m:%Y", tmOraTemp);
                }
                printf("%s\n\n", "AGGREGATO RICHIESTO:");
                sprintf(filename, "%s%i%s%c_%i_%s_%s.txt", "./txtPeer/", myInfo.porta,"/aggr_",aggr, tipo, bound1, bound2);

                fd = fopen(filename, "w");
                if(fd == NULL){
                        sprintf(directory,"%s%i" ,"./txtPeer/",myInfo.porta);
                        mkdir(directory, 0777);
                        fd = fopen(filename, "w");
		}
                        if(modalita){
                                fprintf(fd, "Aggregato %c %i %d", aggr, tipo, sendAggregato.totale);
                                printf("Aggregato: %c %i %d\n\n", aggr, tipo, sendAggregato.totale);
                        }
                        else{
                                fprintf(fd, "Aggregato %c %i %d", aggr, tipo, totale);
                                printf( "Aggregato: %c %i %d\n\n", aggr, tipo, totale);
                        }
                        fclose(fd);

        }

}

int controllaAggr(char*bound1, char*bound2, char aggr, int tipo){
        FILE *fd;
        char filename[MAX_FILE];
        char aggregato[11];
        int totale;
        char bound2_temp[MAX_DATA+20];
        strcpy(bound2_temp, bound2);
        trovaTempo();
        if(strcmp(bound2,"*")==0){
                strftime(bound2_temp, sizeof(Peer_entry.date), "%d:%m:%Y", tmOraTemp);
        }
        sprintf(filename, "%s%i%s%c_%i_%s_%s.txt", "./txtPeer/", myInfo.porta,"/aggr_",aggr, tipo, bound1, bound2_temp);

        fd = fopen(filename, "r");
        if(fd == NULL){
                printf("Non ho l'aggregato richiesto\n" );
                return 0;
        }
        else{
                printf("Possiedo l'aggregato\n\n" );
        }

        if(aggr=='v'){
                printf("AGGREGATO RICHIESTO: \n\n" );
                while(fscanf(fd, "%s %i\n", bound2_temp, &totale) != EOF) {
                        printf( "%s %i\n", bound2_temp, totale);
                }

        }
        else{
                printf("AGGREGATO RICHIESTO: \n\n" );
                fscanf(fd, "%s %c %i %i\n", aggregato, &aggr, &tipo, &totale );
                printf( "%s %c %i %i\n", aggregato, aggr, tipo, totale );
                sendAggregato.aggr=aggr;
        }
        fclose(fd);
        return 1;
}

//Controllo che un'entry non sia gia' nel file
int entryPresente(char* entry){
        FILE *fd;
        char filename[MAX_FILE];
        char tm[MAX_TEMPO];
        int t;
        int q;
        int p;
        char e[630];
        int ret;

        trovaTempo();

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", "entries", myInfo.porta);

        fd = fopen(filename, "r");
        if(fd == NULL)
                return 0;
        //Scorro tutto il file per vedere se la trovo
        while(fscanf(fd, "%s %i %i %i", tm, &t, &q, &p) != EOF){
                ret = sprintf(e, "%s %i %i %i", tm, t, q, p);
                e[ret] = '\0';
                if(strcmp(entry, e) == 0){
                        printf("Entry gia' presente, da non inserire\n\n");
                        return 1;
                }
        }
        //Se arrivo qui non c'e'

        return 0;
}

int inviaEntriesMancanti(int req_port, int tipo, char bound1[MAX_DATA], char bound2[MAX_DATA], char* header){


        FILE *fd;
        char filename[MAX_FILE];
        char whole_entry[MAX_UPDATEENTRY];
	char temp_buffer[MAX_ENTRY];
        int ret;
        int porta;
        int quanti=0;
        int tipo_temp=-1;
        char date[11];
        char bound1_temp[MAX_DATA];
        char bound2_temp[MAX_DATA];
        //char rispostaBuffer[MAX_SOCKET_RECV];

        dateToConvert.tm_hour = dateToConvert.tm_min = dateToConvert.tm_sec = 0;

        int appoggioData[2][3];

        sprintf(filename, "%s_%i.txt", "./txtPeer/entries", myInfo.porta);


        fd = fopen(filename, "r");
        if(fd == NULL){
                printf("Nessuna entry\n");
                return 0;
        }
        //Controllo sulla prima data


        if(strcmp(bound1, "*") != 0) {
                sscanf(bound1, "%d:%d:%d", &appoggioData[0][2], &appoggioData[0][1], &appoggioData[0][0]);
                sprintf(bound1_temp, "%i_%i_%i",appoggioData[0][2], appoggioData[0][1], appoggioData[0][0]);
                strptime(bound1_temp, "%d_%m_%Y", &dateToConvert);
                min_date_given = mktime(&dateToConvert)+TZ;\


        }

        if(strcmp(bound2, "*") != 0) {
                sscanf(bound2, "%d:%d:%d", &appoggioData[1][2], &appoggioData[1][1], &appoggioData[1][0]);
                sprintf(bound2_temp, "%i_%i_%i", appoggioData[1][2], appoggioData[1][1], appoggioData[1][0]);
                strptime(bound2_temp, "%d_%m_%Y", &dateToConvert);
                max_date_given = mktime(&dateToConvert)+TZ;
        }

        while(fscanf(fd, "%s %i %i %i\n", date, &tipo_temp, &quanti, &porta) != EOF|| miDevoFermare) {
        //conversione data prelevata
                strptime(date, "%d_%m_%Y", &dateToConvert);
                date_tmp = mktime(&dateToConvert)+3600;
                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") != 0){
                                if((difftime(min_date_given, date_tmp) <= 0) && (difftime(max_date_given, date_tmp) > 0)){
                                        if(tipo==tipo_temp){
                                                ret = sprintf(whole_entry, "%s  %i %s %i %i", header, tipo, date, quanti, porta);
                                                whole_entry[ret] = '\0';
                                                inviaUDP(sd, whole_entry, ret, req_port);
						riceviUDP(sd, temp_buffer, MAX_SOMMA);
						if(strcmp(temp_buffer, "FLOODST0") == 0){
			                                printf("%s\n","Il richiedente ha tutte le entries, mi devo fermare\n\n>");
			                                miDevoFermare=1;

			                        }
                                                if(miDevoFermare){
                                                        return 1;
                                                }
                                        }
                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") != 0){
                                if(difftime(max_date_given, date_tmp) > 0){
                                        if(tipo==tipo_temp){
                                                ret = sprintf(whole_entry, "%s  %i %s %i %i", header, tipo, date, quanti, porta);
                                                whole_entry[ret] = '\0';
                                                inviaUDP(sd, whole_entry, ret, req_port);
						riceviUDP(sd, temp_buffer, MAX_SOMMA);
						if(strcmp(temp_buffer, "FLOODST0") == 0){
			                                printf("%s\n","Il richiedente ha tutte le entries, mi devo fermare\n\n>");
			                                miDevoFermare=1;

			                        }
                                                if(miDevoFermare){
                                                        return 1;
                                                }
                                        }
                                }
                        }
                        if(strcmp(bound1, "*") != 0 && strcmp(bound2, "*") == 0){
                                if((difftime(min_date_given, date_tmp) <= 0)){
                                        if(tipo==tipo_temp){
                                                ret = sprintf(whole_entry, "%s  %i %s %i %i", header, tipo, date, quanti, porta);
                                                whole_entry[ret] = '\0';
                                                inviaUDP(sd, whole_entry, ret, req_port);
						riceviUDP(sd, temp_buffer, MAX_SOMMA);
						if(strcmp(temp_buffer, "FLOODST0") == 0){
			                                printf("%s\n","Il richiedente ha tutte le entries, mi devo fermare\n\n>");
			                                miDevoFermare=1;

			                        }
                                                if(miDevoFermare){
                                                        return 1;
                                                }
                                        }
                                }
                        }
                        if(strcmp(bound1, "*") == 0 && strcmp(bound2, "*") == 0){
                                if(tipo==tipo_temp){
                                        ret = sprintf(whole_entry, "%s  %i %s %i %i", header, tipo, date, quanti, porta);
                                        whole_entry[ret] = '\0';
                                        inviaUDP(sd, whole_entry, ret, req_port);
					riceviUDP(sd, temp_buffer, MAX_SOMMA);
					if(strcmp(temp_buffer, "FLOODST0") == 0){
						printf("%s\n","Il richiedente ha tutte le entries, mi devo fermare\n\n>");
						miDevoFermare=1;

					}
                                        if(miDevoFermare){
                                                return 1;
                                        }
                                }
                        }
        }

        fclose(fd);
        if(miDevoFermare)
                return 1;
        printf("Non ho tutte le entries necessarie, inoltro\n");
        return 0; // Se arrivo qui il richiedente ha bisogno di altre Entries
}




/***************** FINE GESTORE FILE********************/

//Valuta Chiusura File Odietrno


void stampaRisultati(){
        printf("QUI DEVO STAMPARE AGGREGATO\n");
}
void checkTime() {	//controlla se bisogna chiudere il file odierno
        struct tm* tmOraTemp;
        time_t tempoOraTemp;

        tempoOraTemp = time(NULL);
        tmOraTemp = gmtime(&tempoOraTemp);
        if((tmOraTemp->tm_hour == 18 && tmOraTemp->tm_min == 0) || debug==1){
		inviaEntryDS();
		printf("Registro della data odierna chiuso\n");
        }

}
