#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#define BUFFERSIZE 512

int total_bytes = 0;

//signal handler
void sig_handler(int signo)
{
  if (signo == SIGINT){
    char msg[BUFFERSIZE];
    sprintf(msg,"Received SIGINT, downloaded %d bytes so far\n",total_bytes);
    fprintf(stderr, "%s",msg); 
    exit(0);
    }
}

void error(char *msg)
{
    perror(msg);
    exit(0);
}


int main(int argc, char *argv[])
{
    int portno, mode;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 5) {
       fprintf(stderr,"usage %s filename hostname port mode\n", argv[0]);
       exit(0);
    }

    /* processing info */
    char *filename=argv[1];
    char *hostname=argv[2];
    portno = atoi(argv[3]);
    
    //fixing the mode
    if(strcmp(argv[4],"display")==0){
        mode=1;
    } 
    else if(strcmp(argv[4],"nodisplay")==0) {
        mode=0;
    }
    else{
        fprintf(stderr,"allowed modes are 'display' and 'nodisplay'\n");
        exit(0);
    }

    //server datastructure initialized
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    //------------------------------------------------------
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /*signal binding */
    if (signal(SIGINT, sig_handler) == SIG_ERR)
                    printf("\ncan't catch SIGINT\n");

                    
    /* connect to server */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    int n = write(sockfd,filename,strlen(filename)); //write filename to socket
    if (n < 0){
        error("ERROR writing to socket");
    } 
    else{
    
        int bytesReceived=0;
        char recvBuffer[BUFFERSIZE], buffer[BUFFERSIZE];
        bzero(recvBuffer,BUFFERSIZE);

        while(1)
        {   
            bytesReceived = recv(sockfd, recvBuffer, BUFFERSIZE,0); //read from socket
            if(bytesReceived<=0)break;
            if(mode==1){
                 printf("%s \n", recvBuffer);  //print if mode is display
            }

            total_bytes+=bytesReceived;  //total byte computation
            bzero(recvBuffer,BUFFERSIZE); 
        }

        if(total_bytes == 0)
        {
            printf("Error:%s No bytes received\n",filename);
            exit(0);
        }
        else if(bytesReceived < 0)
        {
            error("Read Error");
        }
            
    }
    close(sockfd);                  //close socket
     
}


    

   

