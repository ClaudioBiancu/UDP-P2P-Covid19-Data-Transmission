//File contenente costanti e funzioni di utilita' per i peer
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
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
#define MAX_DATA 10
#define MAX_TEMPO 8
#define MAX_FILE 31
#define ATTESA_BOOT 5
#define MAX_SOMMA 19
#define MAX_PEERS 100
#define MAX_ENTRY 16


char dataOra[MAX_DATA+1];
char tempoOra[MAX_TEMPO+1];

/***************** FINE COSTANTI********************/

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

//Recupera la data e l'ora correnti e le inserisce nelle variabili globali lato client
void trovaTempo(){
        time_t tempoOraTemp;
        struct tm* tmOraTemp;

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
        time_t tempoOraTemp;
        struct tm* tmOraTemp;
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
        /*if(strcmp(date1, "*") == 0 && strcmp(date2, "*") == 0){
                printf("Non si puo' inserire due volte *.\nLasciare vuoti i campi data se si desidera l'intero periodo\n");
                return 0;
        }*/
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


        tempoOraTemp = time(NULL);
        tmOraTemp = gmtime(&tempoOraTemp);

        if((oggi(date1) || oggi(date2) || strcmp(date2, "*") == 0) && (tmOraTemp->tm_hour < 18)){
                printf("Register di oggi non ancora chiuso, aspetta le 18 per eseguiere una get\n");
                return 0;
        }


        //Tutto ok
        return 1;
}




/***************** GESTORE FILE********************/

void inserisciEntry(char tipo, int quanto, int miaPorta){
        FILE *fd;
        char filename[MAX_FILE];

        trovaTempo();

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", dataOra, miaPorta);

        printf("Filename: %s\n", filename);

        fd = fopen(filename, "a");
        fprintf(fd, "%s %c %d %d;\n", tempoOra, tipo, quanto, miaPorta);//******************************************************************************
        fclose(fd);

        printf("Entry inserita!\n\n");
}

void inserisciEntryStringa(char* entry, int miaPorta){
        FILE *fd;
        char filename[MAX_FILE];

        trovaTempo();

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", dataOra, miaPorta);

        fd = fopen(filename, "a");
        fprintf(fd, "%s\n", entry);
        fclose(fd);

        printf("Entry inserita!\n");
}

int contaEntries(char tipo, int miaPorta){
        FILE *fd;
        char filename[MAX_FILE];
        int tot;
        char entry_type;
        char u_time[MAX_TEMPO];
        int num;
        char tot_peers[6*MAX_PEERS];

        tot = 0;

        trovaTempo();

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", dataOra, miaPorta);

        //printf("Filename: %s\n", filename);

        fd = fopen(filename, "r");
        if(fd == NULL)
                return 0;
        else {
                while(fscanf(fd, "%s %c %d %s\n", u_time, &entry_type, &num, tot_peers) == 4)
                        tot += (tipo == entry_type || tipo == 'a');
        }
        fclose(fd);
        return tot;
        }

int sommaEntries(char tipo, int miaPorta){
        FILE *fd;
        char filename[MAX_FILE];
        int tot;
        char entry_type;
        char u_time[MAX_TEMPO];
        int num;
        char tot_peers[6*MAX_PEERS];
        tot = 0;

        trovaTempo();

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", dataOra, miaPorta);

        fd = fopen(filename, "r");
        if(fd == NULL)
                return 0;
        else {
                while(fscanf(fd, "%s %c %d %s\n", u_time, &entry_type, &num, tot_peers) == 4)
                if(entry_type == tipo)
                        tot += num;
        }
        fclose(fd);
        return tot;
}

void scriviAggr(int count, int sum, char tipo, int miaPorta){
        FILE *fd;
        char filename[MAX_FILE];
        printf("Numero di ");
        if(tipo == 't')
                printf("tamponi");
        else
                printf("nuovi casi");
                printf(" odierni: %d\n", sum);

        trovaTempo();
        sprintf(filename, "%s%c_%s_%d.txt", "./txtPeer/aggr_", tipo, dataOra, miaPorta);

        fd = fopen(filename, "w");
        fprintf(fd, "%d %d", count, sum);
        fclose(fd);
}

int controllaAggr(int entries, char tipo, int miaPorta){
        FILE *fd;
        char filename[MAX_FILE];
        int count;
        int sum;

        trovaTempo();

        sprintf(filename, "%s%c_%s_%d.txt", "./txtPeer/aggr_", tipo, dataOra, miaPorta);

        fd = fopen(filename, "r");
        if(fd == NULL)
                return 0;

        fscanf(fd, "%d %d", &count, &sum);
        fclose(fd);
        return (count == entries) ? sum : 0;
}

//Controllo che un'entry non sia gia' nel file
int entryPresente(char* entry, int miaPorta){
        FILE *fd;
        char filename[MAX_FILE];
        char tm[MAX_TEMPO];
        char t;
        int q;
        char p[6*MAX_PEERS];
        char e[630];
        int ret;

        trovaTempo();

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", dataOra, miaPorta);

        fd = fopen(filename, "r");
        if(fd == NULL)
                return 0;
        //Scorro tutto il file per vedere se la trovo
        while(fscanf(fd, "%s %c %d %s", tm, &t, &q, p) != EOF){
                ret = sprintf(e, "%s %c %d %s", tm, t, q, p);
                e[ret] = '\0';
                if(strcmp(entry, e) == 0){
                        printf("Entry gia' presente, da non inserire\n");
                        return 1;
                }
        }
        //Se arrivo qui non c'e'

        return 0;
}

void inviaEntriesMancanti(int req_port, char tipo, char* header, int miaPorta, int sd){
        FILE *fd, *temp;
        char filename[MAX_FILE];
        char e_time[MAX_TEMPO];
        char e_type;
        int e_quantity;
        char e_peers[6*MAX_PEERS];
        char whole_entry[MAX_ENTRY];
        char check[7];
        int ret;

        ret = sprintf(check, "%d;", req_port);
        check[ret] = '\0';

        trovaTempo();

        sprintf(filename, "%s%s_%d.txt", "./txtPeer/", dataOra, miaPorta);

        printf("Scorro tutte le entries\n");

        fd = fopen(filename, "r");
        if(fd == NULL){
                printf("Nessuna entry\n");
                return;
        }
        //Quando trovo un'entry devo aggiornarla e riscriverla
        temp = fopen("./txtPeer/temp.txt", "w");
        //Scorro tutte le entries
        while(fscanf(fd, "%s %c %d %s", e_time, &e_type, &e_quantity, e_peers) != EOF){
                //Se ne trovo una del tipo richiesto e non posseduta dal richiedente
                if((e_type == tipo || tipo == 'a') && strstr(e_peers, check) == NULL){
                        printf("Trovata entry da inviare\n");
                        //Aggiungo il suo numero di porta tra i peer che possiedono quella entry
                        strcat(e_peers, check);
                        //Gliela mando
                        ret = sprintf(whole_entry, "%s %s %c %d %s", header, e_time, e_type, e_quantity, e_peers);
                        whole_entry[ret] = '\0';
                        inviaUDP(sd, whole_entry, ret, req_port);
                }
                //Se quell'entry e' gia' presente la copio e basta
                else {
                        ret = sprintf(whole_entry, "%s %s %c %d %s", header, e_time, e_type, e_quantity, e_peers);
                        whole_entry[ret] = '\0';
                }
                fprintf(temp, "%s\n", whole_entry+9);
        }

        fclose(fd);
        fclose(temp);
        remove(filename);
        rename("./txtPeer/temp.txt", filename);

        printf("Fine delle entries da inviare\n");
}

void inviaEntriesVicini(int sd, int porta, int vicino1, int vicino2){
    FILE *fd;
    char filename[MAX_FILE];
    char e_time[MAX_TEMPO];
    char e_type;
    int e_quantity;
    char e_peers[6*MAX_PEERS];
    char whole_entry[MAX_ENTRY];
    char check1[7];
    char check2[7];
    char check3[13];
    int ret;
    char *bool1, *bool2;

    if(vicino1 == -1)
        return;

    if(vicino2 == -1){
        inviaEntriesMancanti(vicino1, 'a', "EP2P_NEW", porta, sd);
        return;
    }

    ret = sprintf(check1, "%d;", vicino1);
    check1[ret] = '\0';
    ret = sprintf(check2, "%d;", vicino2);
    check2[ret] = '\0';
    ret = sprintf(check3, "%d;%d;", vicino1, vicino2);
    check3[ret] = '\0';

    trovaTempo();

    sprintf(filename, "%s%s_%d.txt", "./txtPeer/", dataOra, porta);

    printf("Scorro tutte le entries\n");

    fd = fopen(filename, "r");
    if(fd == NULL){
        printf("Nessuna entry\n");
        return;
    }
    //Scorro tutte le entries
    while(fscanf(fd, "%s %c %d %s", e_time, &e_type, &e_quantity, e_peers) != EOF){
            printf("Trovata entry da inviare\n");
            //Decido a quale peer va inviata
            bool1 = strstr(e_peers, check1);
            bool2 = strstr(e_peers, check2);

            //Caso 1: manca a entrambi
            if(bool1 == NULL && bool2 == NULL)
                strcat(e_peers, check3);
            //Caso 2: manca a 2
            if(bool1 != NULL && bool2 == NULL)
                strcat(e_peers, check2);
            //Caso 3: manca a 1
            if(bool1 == NULL && bool2 != NULL)
                strcat(e_peers, check1);

            //Mando la entry a chi di dovere
            ret = sprintf(whole_entry, "%s %s %c %d %s", "EP2P_NEW", e_time, e_type, e_quantity, e_peers);
            whole_entry[ret] = '\0';
            if(bool1 == NULL)
                inviaUDP(sd, whole_entry, ret, vicino1);
            if(bool2 == NULL)
                inviaUDP(sd, whole_entry, ret, vicino2);

    }

    fclose(fd);

    printf("Fine delle entries da inviare\n");
}


/***************** FINE GESTORE FILE********************/
