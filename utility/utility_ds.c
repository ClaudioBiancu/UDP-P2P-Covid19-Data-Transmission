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
//Torna utile quando devo scorrere una stringa e mi serve un delimitatore
#define SPAZIO " \n"
//Dimensione massima del buffer
#define BUFLEN 1024

#define LOCALHOST "127.0.0.1"

#define MAX_SOCKET_RECV 630
#define MAX_TIPO 8

/***************** FINE COSTANTI********************/
/*****************HEADER**************************/
int creaSocketAscolto(struct sockaddr_in* my_addr, int porta);
int riceviUDP();
/***************** FINE HEADER********************/


int creaSocketAscolto(struct sockaddr_in* my_addr,int porta) {
  int sd;
  int ret;
  /* Creazione socket UDP */
  sd = socket(AF_INET, SOCK_DGRAM, 0);
  /* Creazione indirizzo */
  memset(my_addr, 0, sizeof(*my_addr)); // Per convenzione
  my_addr->sin_family = AF_INET ;
  my_addr->sin_port = htons(porta);
  //my_addr.sin_addr.s_addr = INADDR_ANY;
  inet_pton(AF_INET, LOCALHOST, &my_addr->sin_addr);

  /*Aggancio*/
  ret = bind(sd, (struct sockaddr*)my_addr, sizeof(*my_addr));
  if( ret < 0 ){
      perror("Bind non riuscita\n");
      exit(0);
  }
  printf("Discovery Server in ascolto sulla porta:%d\n\n", porta);
 /* while(1){
      do{
        ret = recvfrom(sd, buffer, BUFLEN, 0,(struct sockaddr*)&cl_addr, &addrlen);
        buffer[BUFLEN]='\0';
        printf("%s\n", buffer);
        if(ret<0){

        }
}*/

return sd;

}

int riceviUDP(int socket, char* buffer, int buff_l){
  struct sockaddr_in send_addr;
    socklen_t send_addr_len;

    send_addr_len = sizeof(send_addr);

    recvfrom(socket, buffer, buff_l, 0, (struct sockaddr*)&send_addr, &send_addr_len);

    return ntohs(send_addr.sin_port);

}
