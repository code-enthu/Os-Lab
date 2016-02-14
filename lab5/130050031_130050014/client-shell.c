#include  <stdio.h>
#include  <sys/types.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
#define MAX_BGPROCESS 70 // more than 64 to be safe
#define MAX_FGPROCESS 70 

int set_server = 0;//for debug purposes, 1 = debug, 0 = normal

//global variables-----------------------------------------
char server_ip[MAX_INPUT_SIZE];
char server_port[MAX_INPUT_SIZE];
int pid_bg[MAX_BGPROCESS];
int pid_fg[MAX_FGPROCESS];
int kill_sq = 0;// Ctrl + C,to stop spawning sequential downloads,1=true,0=false;

//----------------------------------------------------------
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';
      if (tokenIndex != 0){
	tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
	strcpy(tokens[tokenNo++], token);
	tokenIndex = 0; 
      }
    } else {
      token[tokenIndex++] = readChar;
    }
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}
//----------------------------------------------------------
//exit function, shell preparing to exit
void myexit(char ** tokens){
	if(tokens[1]!=NULL){
		printf("Error: Wrong number of arguments\n");
		return;
	}

	int i = 0;
	int status;
	for(i = 0 ; i < MAX_BGPROCESS; i++){
		if(pid_bg[i] != -1){
			kill(pid_bg[i],SIGINT);// kill bg processes
		}
	}
	for(i = 0 ; i < MAX_BGPROCESS; i++){
		if(pid_bg[i] != -1){
			waitpid(pid_bg[i],&status,0);
			pid_bg[i] = -1;// reset after reaping
		}
	}
	exit(0);
}
//----------------------------------------------------------
//signal handler
//SIGKILL (die! now!), SIGTERM (please, go away), SIGHUP (modem hangup), SIGINT (^C), SIGQUIT (^\), etc.
//Many signals have as default action to kill the target. (Sometimes with an additional core dump, when such is allowed by rlimit.) 
//The signals SIGCHLD and SIGWINCH are ignored by default. 
//All except SIGKILL and SIGSTOP can be caught or ignored or blocked.

void sig_handler(int signo){
	int status;
	int num_process_killed = 0;
	kill_sq = 1; //stop spawning seq downloading
	if (signo == SIGINT){
		int i = 0;
		int status;
		for(i = 0 ; i < MAX_FGPROCESS; i++){
			if(pid_fg[i] != -1){
				kill(pid_fg[i],SIGINT); // kill fg processes
				num_process_killed++;
			}
		}
		for(i = 0 ; i < MAX_FGPROCESS; i++){
			if(pid_fg[i] != -1){
				waitpid(pid_fg[i],&status,0);
				pid_fg[i] = -1;// reset after reaping
			}
		}
	}
	if(num_process_killed == 0){
		printf("\nHello>");//go to next line when user presses Ctrl+C while nothing is executing
    	fflush(stdout);
	}
	return;
}
//----------------------------------------------------------
//If successful, waitpid returns the process ID of the terminated process whose status was reported. 
//If unsuccessful, a -1 is returned.
//WUNTRACED causes the call to waitpid to return status information for a specified process that has either stopped or terminated. 
//Normally, status information is returned only for terminated processes.

//this function runs continuously in a while loop, constatntly reaping background processes
void reap_bgprocess(){
	int i = 0;
	int status;
	for(i = 0 ; i < MAX_BGPROCESS; i++){
		if(pid_bg[i] != -1){
			int endid = waitpid(pid_bg[i], &status, WNOHANG|WUNTRACED);//reap bg
			if(endid == pid_bg[i]){
				printf("Hello>Background process with pid:%d completed successfully!\n",pid_bg[i]);
				pid_bg[i] = -1;
			}
		} 
	}
	return;
}


//----------------------------------------------------------
//functions to maintain pid of fg processes
void insert_fg(int pid){
	int i = 0;
	for(i = 0; i< MAX_FGPROCESS; i++){
		if(pid_fg[i] == -1){
			pid_fg[i] = pid;
			break;
		}
	}
	return;
}

void free_fg(int pid){
	int i = 0;
	for(i = 0; i< MAX_FGPROCESS; i++){
		if(pid_fg[i] == pid){
			pid_fg[i] = -1;
			break;
		}
	}
	return;
}
//----------------------------------------------------------
//generic function to execute general shell commands by running binary files from /bin
void generic(char* command, char ** tokens){
  int pid;
  char path[100] = "/bin/";
  strcat(path,command);
  pid = fork();
  if(pid == 0){
    if(execv(path,tokens) < 0){
    	fprintf(stderr,"Error: exec failed, no such command\n");
    	exit(1);
    }
  }

  else{
    int status;
    waitpid(pid, &status,0);// reaping
  }

  return;
}
//----------------------------------------------------------
//change directory using chdir() syscall
void cd(char ** tokens){
  if(tokens[2]!=NULL || tokens[1]==NULL){
    fprintf(stderr,"Error: Wrong number of arguments; \n");
    return;
  }

  if(chdir(tokens[1])==0){
  }
  else {
    fprintf(stderr,"Error: filepath incorrect\n");
  }
}
//----------------------------------------------------------
//setting server data
void server(char ** tokens){
	if(tokens[1]==NULL || tokens[2]==NULL || tokens[3]!=NULL){
    	fprintf(stderr,"Error: Wrong number of arguments\n");
    	return;
 	}
 	
	bzero(server_ip,MAX_INPUT_SIZE);
	bzero(server_port,MAX_INPUT_SIZE);
	strcpy(server_ip,tokens[1]);
	strcpy(server_port,tokens[2]);

	return;
}
//----------------------------------------------------------
//getfile with redirect directive
void getfl_redirect(char ** tokens){
	if(tokens[3]==NULL || tokens[4] != NULL){
	  fprintf(stderr,"Error: Wrong number of arguments\n");
	  return;
	}

	int pid=fork();
	char* argv[] = {"./get-one-file-sig",tokens[1],server_ip,server_port,"display",NULL};
	if(pid==0){
		close(1);//close output file descriptor of child
		FILE * fd;
  		fd = fopen (tokens[3],"w");// stdout of child now points to this file
		int value = execv("./get-one-file-sig",argv); // load binary
		if(value < 0){
			fprintf(stderr,"Error: exec failed\n");
			exit(1);
		}
	}
	else{
	   	insert_fg(pid); // maintain pid in fg array 
  		int status;
  		waitpid(pid,&status, 0); // reap
  		free_fg(pid); //delete from array
	}
	
	return;
}
//----------------------------------------------------------
void getfl_pipe(char ** tokens){
	if(tokens[3]==NULL){
	  fprintf(stderr,"Error: Wrong number of arguments\n");
	  return;
	}

	char* argv1[] = {"./get-one-file-sig",tokens[1],server_ip,server_port,"display",NULL};
	int p[2];
	pipe(p); // pipe array!
	int pid1=fork(); //fork first child (getfl)
	if(pid1==0){
		close(1);
		int write_end=dup(p[1]); //
		close(p[0]);
		close(p[1]);
		if(write_end!=1){
			fprintf(stderr,"Error: Pipe failure write end\n");
			exit(1);
		}
		
		if(execv(argv1[0],argv1)<0){ //load binary
			fprintf(stderr,"Error:Execv falied\n");
			exit(1);
		};
	}
	else{
		/* create token for command following pipe */

		int i;
		int size = 0;
		char** argv2 = (char **)malloc(MAX_NUM_TOKENS*sizeof(char *));
		for(i=3;tokens[i]!=NULL;i++){
			argv2[i-3] = (char *)malloc(MAX_NUM_TOKENS*sizeof(char));
			bzero(argv2[i-3],MAX_TOKEN_SIZE);	               	
            strcpy(argv2[i-3], tokens[i]);
			size++;
		}
		
		int pid2=fork(); // second child forked, its input is output of first child
		if(pid2==0){
			// read end of pipe assigned to fd0 
			close(0);
			int read_end=dup(p[0]);
			close(p[0]);
			close(p[1]);
			if(read_end!=0){
				fprintf(stderr,"Error: Pipe failure read end\n");
				exit(1);
			}
			if(execvp(argv2[0],argv2)<0){ //load binary
				fprintf(stderr,"Error: exec failed, no such command\n");
				exit(1);
			}
		}
		else{
			//wait for both processes
			close(p[0]);
			close(p[1]);
			insert_fg(pid1); // insert in fg array
			insert_fg(pid2);			
			int status1,status2;
			waitpid(pid1,&status1,0); //reap
			waitpid(pid2,&status2,0);
			free_fg(pid1); //delete from fg array
			free_fg(pid2);

		}

		// Freeing the allocated memory	
		for(i=0;i < size;i++){
			free(argv2[i]);
		}
		free(argv2);
	}
	
	return;
}
//----------------------------------------------------------
void getfl(char ** tokens){
	if((strcmp(server_ip,"NULL")==0) || (strcmp(server_port,"NULL")==0)){
		fprintf(stderr,"Error: Server data uninitialised\n");
		return;
	} //checking server data structure

	if(tokens[2] != NULL){
		if(tokens[2][0] == '>'){
			getfl_redirect(tokens);
			return;
		} //redirection function called
		
		if(tokens[2][0] == '|'){
			getfl_pipe(tokens);
			return;
		} // pipe handling required
	}
	

	if(tokens[1] == NULL || tokens[2] != NULL){
		fprintf(stderr,"Error: Wrong number of arguments\n");
		return;
	} //error check

	int pid = fork();
	char* argv[] = {"./get-one-file-sig",tokens[1],server_ip,server_port,"display", NULL};
	
	if(pid == 0){
		int value = execv(argv[0],argv); //load binary
    	if(value < 0){
    		fprintf(stderr,"Error: exec failed\n");
    		exit(1);
    	}
  	}

  	else{
    	insert_fg(pid); // insert in fg_array
  		int status;
  		waitpid(pid,&status, 0); // reap
  		free_fg(pid); // delete from fg array
  	}


}
//----------------------------------------------------------
void getsq(char ** tokens){
	
	if(tokens[1]==NULL){
		fprintf(stderr,"Error: Wrong number of arguments\n");
		return;
	}
	if((strcmp(server_ip,"NULL")==0) || (strcmp(server_port,"NULL")==0)){
		fprintf(stderr,"Error: Server data uninitialised\n");
		return;
	}
	int i=1;
	while(tokens[i]!=NULL && kill_sq == 0 ){ //kill sq makes sure signal is not pressed during execution
		int pid = fork();
		char* argv[] = {"./get-one-file-sig",tokens[i],server_ip,server_port,"nodisplay", NULL}; //load binary
		if(pid == 0){
			if(execv(argv[0],argv)< 0){
	    		fprintf(stderr,"Error: exec failed\n");
	    		exit(1);
	    	}
	  	}
  		else{
  			insert_fg(pid); // insert in fg_array
  			int status;
  			waitpid(pid,&status, 0); // reap
  			free_fg(pid); // delete from fg array
  			i++;
  		}
  	}
  	return;
}
//----------------------------------------------------------
void getpl(char ** tokens){
	
	if(tokens[1]==NULL){
		fprintf(stderr,"Error: Wrong number of arguments\n");
		return;
	}
	if((strcmp(server_ip,"NULL")==0) || (strcmp(server_port,"NULL")==0)){
		fprintf(stderr,"Error: Server data uninitialised\n");
		return;
	}
	int i=1;
	int pid[MAX_NUM_TOKENS];

	while(tokens[i]!=NULL){
		pid[i] = fork();
		char* argv[] = {"./get-one-file-sig",tokens[i],server_ip,server_port,"nodisplay", NULL}; //load binary
		if(pid[i] == 0){
			if(execv(argv[0],argv)< 0){
	    		fprintf(stderr,"Error: exec failed\n");
	    		exit(1);
	    	}
	  	}
  		else{
  			insert_fg(pid[i]); // insert in fg_array, no waiting required reaping done later
  			i++;
  		}
  	}
  	int tot_files=i-1; 
  	i=1;
  	while(i<=tot_files){
  		int status;
  		waitpid(pid[i],&status, 0); // reap
  		free_fg(pid[i]); // delete from fg array
  		i++;
  	}
  	return;

}
//----------------------------------------------------------
void getbg(char ** tokens){
	
	if(tokens[1]==NULL || tokens[2] != NULL){
		fprintf(stderr,"Error: Wrong number of arguments\n");
		return;
	}
	if((strcmp(server_ip,"NULL")==0) || (strcmp(server_port,"NULL")==0)){
		fprintf(stderr,"Error: Server data uninitialised\n");
		return;
	}

	int pid = fork();
	char* argv[] = {"./get-one-file-sig",tokens[1],server_ip,server_port,"nodisplay", NULL};
	if(pid == 0){
		setpgid(0,0);
		if(execv(argv[0],argv)< 0){ // load binary
    		fprintf(stderr,"Error: exec failed\n");
    		exit(1);
    	}
  	}
	else{
		int i;
		for(i = 0; i< MAX_BGPROCESS; i++){
			if(pid_bg[i] == -1){
				pid_bg[i] = pid;
				break;
			}
		}// no reaping required here, reaping done in main() 
	}

  	return;
}

//----------------------------------------------------------
// function to parse the tokens and call appropriate handler
void command(char ** tokens){
	char* first = tokens[0];
	if(first == NULL){
		return;
	} // when user just presses ENTER

	if(strcmp(first,"cd") == 0){
		cd(tokens);
	}	

	else if(strcmp(first,"server") == 0){
		server(tokens);
	}
	else if(strcmp(first,"getfl") == 0){
		getfl(tokens);
	}
	else if(strcmp(first,"getsq") == 0){
		kill_sq = 0; //reset kill_sq if kill_sq is 1
		getsq(tokens);
	}
	else if(strcmp(first,"getpl") == 0){
		getpl(tokens);
	}
	else if(strcmp(first,"getbg") == 0){
		getbg(tokens);
	}
	else if(strcmp(first,"exit") == 0){
		myexit(tokens);
	}
	else{
		generic(tokens[0],tokens);
	}

	return;
    
}
//----------------------------------------------------------


//Order of fork and argv doesn't matter, for consistency convention: fork -> argv
int main()
{
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;
	strcpy(server_ip,"NULL");
	strcpy(server_port,"NULL"); //intialise server data to NULL string

	if(set_server == 1){
		strcpy(server_ip,"localhost");
		strcpy(server_port,"5000");
	} //used for debug purposes, set_server to 0 when not debugging


	for(i = 0 ; i < MAX_BGPROCESS ; i++){
		pid_bg[i] = -1;
		pid_fg[i] = -1;
	}

	/*signal binding */
	if (signal(SIGINT, sig_handler) == SIG_ERR){
		fprintf(stderr,"\ncan't catch SIGINT\n");
	}


	while (1) {           
		reap_bgprocess();
		printf("Hello>");     
		bzero(line, MAX_INPUT_SIZE);
		fgets(line,MAX_INPUT_SIZE,stdin);           
		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);

		command(tokens);

		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		free(tokens);
	}
     

}

                
