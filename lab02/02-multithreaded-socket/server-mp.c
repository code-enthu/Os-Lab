#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

#define BUFFERSIZE 1024
// file read in chunks of 1024 bytes

void error(char *msg)
{
    perror(msg);
    exit(1);
}

// function to send file on the given socket, file name parsing involved
void send_file(int newsockfd, char* msg){
    char filepath[BUFFERSIZE];
    bzero(filepath,BUFFERSIZE);
    int i = 4;
    for(i =4; i < BUFFERSIZE; i++){
    	// code to parse filename, we are looking for "txt" substring
        if(msg[i] == 't' && msg[i+1] == 'x' && msg[i+2] == 't'){
            filepath[i-4] = msg[i];
            filepath[i-4+1] = msg[i+1];
            filepath[i-4+2] = msg[i+2];
            break;
        }
        else{
            filepath[i-4] = msg[i];
        }
    }
    FILE *fp = fopen(filepath,"rb");

    while (1){
        char buffer[BUFFERSIZE]; //file read in chunks of BUFFERSIZE bytes
        bzero(buffer,BUFFERSIZE);
        int bytes_read = fread(buffer,1,BUFFERSIZE,fp);
        int bytes_written;
        if(bytes_read > 0){
            bytes_written = send(newsockfd, buffer, bytes_read,0);
        }

        if(bytes_read < BUFFERSIZE){
            break;
        } // termination, file sent completely
    }
    return;
}

// function to deal with client requests
void processing(int newsockfd){
    char buffer1[BUFFERSIZE];
    bzero(buffer1,BUFFERSIZE);
    int n = read(newsockfd,buffer1,BUFFERSIZE);
    if (n < 0) error("ERROR reading from socket");
    
    //send file on the socket, parse the filename from buffer1
    send_file(newsockfd,buffer1);
    return;
}


int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, clilen;
     char buffer[BUFFERSIZE];
     bzero(buffer,BUFFERSIZE);
     struct sockaddr_in serv_addr, cli_addr;
     int n;


     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

     /* create socket */

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");

     /* fill in port number to listen on. IP address can be anything (INADDR_ANY) */

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     /* bind socket to this port number on this machine */

     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     /* listen for incoming connection requests */

     listen(sockfd,10); // 10 backlogs allowed
     clilen = sizeof(cli_addr);

     while (1){
        int id = getpid();
        /* accept a new request, create a newsockfd */
        while(waitpid(-1,0,WNOHANG) > 0); //reaping child
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newsockfd < 0){
            error("ERROR on accept");
        }

        //creating child process
        int pid = fork();
        if(pid < 0){
            error("ERROR on fork");
        }

        // this is the child process
        if(pid == 0){
            int id = getpid();
            close(sockfd); //close parent's fd from its PCB
            processing(newsockfd); //cater to client's request
            close(newsockfd); //close socket after processing
            exit(0); //exit status
        }
        else{
            close(newsockfd); //close child's socket fd from its PCB
        }
     }

     return 0; 
}
