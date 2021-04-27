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
/*****************COSTANTI**************************/
//Torna utile quando devo scorrere una stringa e mi serve un delimitatore
#define SPAZIO " \n"
//Dimensione massima del buffer
#define BUFLEN 1024

#define LOCALHOST "127.0.0.1"

/***************** FINE COSTANTI********************/

/*****************HEADER**************************/
int creaSocketAscolto(struct sockaddr_in* socket_ascolto, int porta);
/***************** FINE HEADER********************/






//Crea il socket UDP collegandosi al server con indirizzo passato come primo parametro
//e alla porta passata come secondo parametro, restituisce un descrittore di socket
int creaSocketAscolto(struct sockaddr_in* socket_ascolto, int porta) {
    int ret;
    int sd;
    // Creazione socket
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(socket_ascolto, 0, sizeof(*socket_ascolto)); // Pulizia
    socket_ascolto->sin_family = AF_INET;
    socket_ascolto->sin_port = htons(porta);
	   inet_pton(AF_INET, LOCALHOST, &socket_ascolto->sin_addr);

    ret = bind(sd, (struct sockaddr*)socket_ascolto, sizeof(*socket_ascolto));
    if(ret<0){
        perror("Errore in fase di binding");
        exit(0);
    }
    return sd;
}
