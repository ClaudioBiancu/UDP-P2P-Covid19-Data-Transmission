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


/*void riceviUDP(int socket, char* buff, int lunghezza_buffer, int send_port, char* correct_header){
        int ret;
        struct sockaddr_in send_addr;
        socklen_t send_addr_len;
        char temp_buffer[MAX_TIPO];

        send_addr_len = sizeof(send_addr);

        do {
                ret=recvfrom(socket, buff, lunghezza_buffer, 0, (struct sockaddr*)&send_addr, &send_addr_len); //Leggo cosa ho ricevuto
        } while (ret < 0);
        sscanf(buff, "%s", temp_buffer);

        if((ntohs(send_addr.sin_port) == send_port || send_port == -1) && strcmp(correct_header, temp_buffer) == 0){//Opzione per ricevere da tutti gli indirizzi
                printf("Messaggio %s ricevuto correttamente dal mittente %d\n", buff, ntohs(send_addr.sin_port));//Il peer ha ricevuto sicuramente la lista
        }
        else {
                printf("[R] Arrivato un messaggio %s inatteso da %d mentre attendevo %s da ", temp_buffer, ntohs(send_addr.sin_port), correct_header);//Stampo il messaggio di errore giusto
        }
}*/

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
                while(ret<0){
                        ret = sendto(socket, buff, buff_l+1, 0, (struct sockaddr*)&recv_addr, recv_addr_len);
                        if(ret<0){
                                sleep(5);
                                printf("Attendo risposta");
                        }
                }


        printf("Messaggio %s inviato correttamente al destinatario %d\n", buff, recv_port);
}


/***************** GESTORE FILE********************/



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
/***************** FINE GESTORE FILE********************/
