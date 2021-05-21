#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*****************COSTANTI**************************/
#define SPAZIO " \n" //Torna utile quando devo scorrere una stringa e mi serve un delimitatore
#define BUFLEN 1024//Dimensione massima del buffer
#define LOCALHOST "127.0.0.1"//Indirizzo predefinito
#define MAX_SOCKET_RECV 630 //Dimentsione massima messaggio ricevuto
#define MAX_TIPO 8 //Dimensione massima tipo richiesta al ds
#define MAX_LISTA 21 //Dimensione massima lista vicini
#define ACKWAIT 15000//Attesa dell'ack
/****************FINE COSTANTI********************/



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
        printf("Messaggio ricevuto: %s\n", buffer);
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
        printf("Controllo che il Peer non sia gia' stato inserito\n");
    FILE *fd;
    char temp_buffer[INET_ADDRSTRLEN];
    int temp;

    fd = fopen("./bootedPeers.txt", "r");

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
                fp = fopen("./bootedPeers.txt", "w");
                fprintf(fp, "%s %d\n", addr, port);
                fclose(fp);
                return 1;
        }

        //Inserisco ordinatamente nel caso di un peer gia' presente
        if(peersConnessi == 1){

                temp_port[1] = -1;//non utilizzo la porta 1
                fp = fopen("./bootedPeers.txt", "r");
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
                remove("./bootedPeers.txt");
                rename("temp.txt", "./bootedPeers.txt");

                return 1;
        }

        //Inserisco ordinatamente nel caso di due o piu' peer gia' presenti
        else {
                fp = fopen("./bootedPeers.txt", "r");
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
                remove("./bootedPeers.txt");
                rename("temp.txt", "./bootedPeers.txt");

                return 1;
        }

        return -1;
}

//Trova il numero di porta di un peer, data la sua posizione
int trovaPorta(int posizione){ //Restituisce data la posizione il numero della porta richiesta
    FILE *fp;
    int port;
    char temp[INET_ADDRSTRLEN];
    int ret;

    fp = fopen("./bootedPeers.txt", "r");
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

        fd = fopen("./bootedPeers.txt", "r");
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
/***************** FINE GESTORE FILE********************/
