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

#define BUFFERSIZE 1024
#define NUMFILES 10000

int* response_time;
int* files_read;

struct sockaddr_in serv_addr;
struct hostent *server;
struct threadData data;

/* stores data of threads obtained from command line */
struct threadData{
    int think_time,total_time, id ,mode,portno;
    char* hostname;
};


void error(char *msg)
{
    perror(msg);
    exit(0);
}

/* creating socket */
int create_socket( ){
     /* create socket, get sockfd handle */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* connect to server */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    return sockfd;
}

/* reads file from socket fd */
int readFile(int sockfd){
    int bytesReceived=0, total_bytes=0;
    char recvBuffer[BUFFERSIZE], buffer[BUFFERSIZE];
    bzero(recvBuffer,BUFFERSIZE);
    while(1)
    {
        bytesReceived = recv(sockfd, recvBuffer, BUFFERSIZE,0);
        if(bytesReceived<=0)break;                          /* if no bytes for reading , terminate while */
       
        total_bytes+=bytesReceived;  
        bzero(recvBuffer,BUFFERSIZE);
          
    }
    if(bytesReceived == 0){}
    else if(bytesReceived < 0)
    {
        error("Read Error");
    }
    return bytesReceived; 
}

/* runs the main process */
void processing(int id){
    struct timeval start_time,curr_time,start,end;
    int time_elapsed=0;
    
    gettimeofday(&start_time, NULL); 
    while(time_elapsed< data.total_time){
        char  filename[BUFFERSIZE], buffer[BUFFERSIZE], num[BUFFERSIZE];
        bzero(filename,BUFFERSIZE);
        bzero(num,BUFFERSIZE);
        int sockfd=create_socket();
        if(data.mode==1){                   /* generating file name for random mode */
            strcpy(filename,"get files/foo");
            int i=rand()%NUMFILES;
            sprintf(num,"%d",i);
            strcat(filename,num);
            strcat(filename,".txt");

        }
        else {                                  /* generating file name for fixed mode */
            strcpy(filename,"get files/foo");
            int i=id%NUMFILES;
            sprintf(num,"%d",i);
            strcat(filename,num);
            strcat(filename,".txt");        
        }

        gettimeofday(&start,NULL);          /* start time for calculating response time of a file read */
        
        int n = write(sockfd,filename,strlen(filename)); /* write filename to server */
        if (n < 0){
            error("ERROR writing to socket");
        } 
        else{
            int b=readFile(sockfd);  /* reads the file */
            gettimeofday(&end,NULL);
            if(b>=0){
                response_time[id]+=(end.tv_sec - start.tv_sec)*1000000+(end.tv_usec-start.tv_usec);    
            }
        }
        close(sockfd);                  //close socket
        
        files_read[id]++;
        sleep(data.think_time);   /* thread sleeps for think_time */
        gettimeofday(&curr_time,NULL);
        time_elapsed=(curr_time.tv_sec- start_time.tv_sec);  /* total time elapsed */
    }


}



void* thread_fn (void *ptr )
{
    int *id=(int *)ptr;
    processing(*id);
    pthread_exit(0); 
    return NULL;
} 


int main(int argc, char *argv[])
{
    int portno, n;
    int think_time,total_time, nthreads ,mode;
    struct timeval start,end; //to measure experiment time

    char *filename;
    if (argc < 7) {
       fprintf(stderr,"usage %s hostname port threads total_time think_time mode\n", argv[0]);
       exit(0);
    }

    /* processing info */
    portno = atoi(argv[2]);
    nthreads=atoi(argv[3]);
    total_time=atoi(argv[4]);
    think_time=atoi(argv[5]);

    response_time = malloc(nthreads*sizeof(int));
    files_read = malloc(nthreads*sizeof(int));

    if(strcmp(argv[6],"random")==0)mode=1; 
    else mode=0;

    /* thread data initialized */
    data.hostname=argv[1];
    data.portno=portno;
    data.total_time= total_time;
    data.think_time= think_time; 
    data.mode=mode;

    /* fill in server address in sockaddr_in datastructure */

    server = gethostbyname(data.hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(data.portno);


    gettimeofday(&start,NULL);
    pthread_t thread[nthreads];
    int thread_id[nthreads];
    int i;
    for(i=0;i<nthreads;i++){
            response_time[i]=0;
            files_read[i]=0;
            thread_id[i]=i;
            /* thread creation */
            if(pthread_create (&thread[i], NULL, thread_fn, (void *)&thread_id[i])){
                error("Error creating thread\n");
                exit(1);

            }    
    }
    double tot_files=0,tot_response=0;
    /* join threads */
    for(i=0;i<nthreads;i++){
        pthread_join(thread[i],NULL);
        tot_files+=files_read[i];
        tot_response+=response_time[i];
    }
    /* check for memory leaks , free dynamic arrays */
    free(response_time);
    free(files_read);
    gettimeofday(&end,NULL);
    
    /* compute stats */
    double response_time_secs = (double)tot_response/1000000.0;
    data.total_time = end.tv_sec - start.tv_sec;
    double avg_throughput = (double)tot_files/(double)data.total_time;
    double avg_response = (double)response_time_secs/(double)tot_files;
    printf("Done!\n");
    printf("throughput= %lf \n average response time = %lf\n",avg_throughput,avg_response);
    
     
}


    

   

