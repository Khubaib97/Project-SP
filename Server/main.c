#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>

int msgsock;
char buf[500];
int rval;
int ppid;
pthread_t keyb_tid;
char keyb_in[500];
char* arr[50]; //tokens1
char* arr1[50]; //tokens2
char c_addr[50][INET_ADDRSTRLEN];
int ch_pid[50];
int sock_list[50];
char activity[50][10];
int ps_in = 0;

int process_id[50];
char process_name[50][20];
char process_status[50][10];
time_t process_start[50];
time_t process_end[50];
int process_counter = 0;

void close_sock(int sig){
	write(msgsock, "Server Terminated Connection", strlen("Server Terminated Connection"));
	close(msgsock);
	exit(0);
}

void client_term(int sig){
	int pid;
	while ((pid = waitpid(-1, NULL, WNOHANG)) != -1)
    	{
        	int i = 0;
		for(;i<ps_in;i++){
			if(ch_pid[i]==pid){
				break;
			}
		}
		strcpy(activity[i], "INACTIVE");
    	}
}

void process_term(int sig){
	int pid;
	while ((pid = waitpid(-1, NULL, WNOHANG)) != -1)
    	{
        	int i = 0;
		for(;i<process_counter;i++){
			if(process_id[i]==pid){
				break;
			}
		}
		strcpy(process_status[i], "INACTIVE");
		process_end[i] = time(NULL);
    	}
}

void* keyboard_reader(void* args){
	int pid = *((int *)args);
	while(1 && (getpid()==pid)) {
		bzero(keyb_in, 500);
    		int readchars = read(STDIN_FILENO, keyb_in, 500);
    		if(readchars < 0)
        		perror("Reading from std input");
    		if(keyb_in[readchars-1]=='\n'){
			keyb_in[readchars-1] = '\0';
			char* token = strtok(keyb_in, " ");
   	 		int i = 0;

			while(token != NULL){
				arr1[i++] = token;
				token = strtok(NULL, " ");
			}
			arr1[i] = "@";

			if(strcmp("list", arr1[0])==0 || strcmp("LIST", arr1[0])==0){
				i = 0;
				write(STDOUT_FILENO, "IP_Address\tHandler_PID\tSocket_Value\tActive_Status\n", strlen("IP_Address\tHandler_PID\tSocket_Value\tActive_Status\n"));
				for(;i<ps_in;i++){
					write(STDOUT_FILENO, c_addr[i], strlen(c_addr[i]));
					write(STDOUT_FILENO, "\t", 1);
					char str[50];
					sprintf(str, "%d", ch_pid[i]);
					write(STDOUT_FILENO, str, strlen(str));
					write(STDOUT_FILENO, "\t\t", 2);
					char str1[50];
					sprintf(str1, "%d", sock_list[i]);
					write(STDOUT_FILENO, str1, strlen(str1));
					write(STDOUT_FILENO, "\t\t", 2);
					write(STDOUT_FILENO, activity[i], strlen(activity[i]));
					write(STDOUT_FILENO, "\n", 1);
				}
			}
			else if(strcmp("kill", arr1[0])==0 || strcmp("KILL", arr1[0])==0){
				if(strchr(arr1[1], '.')==NULL){
					int pid = 0;
					sscanf(arr1[1], "%d", &pid);
					if(kill(pid, SIGTERM)<0)
						perror("kill");
					else
						write(STDOUT_FILENO, "Client Terminated\n", strlen("Client Terminated\n"));
				}
				else{
					int i = 0;
					while(i<ps_in){
						if(strcmp(arr1[1], c_addr[i])==0){
							i = ch_pid[i];
							break;
						}
						i++;
					}
					if(kill(i, SIGTERM)<0)
						perror("kill");
					else
						write(STDOUT_FILENO, "Client Terminated\n", strlen("Client Terminated\n"));
				}
			}
			else if(strcmp("help", arr1[0])==0 || strcmp("HELP", arr1[0])==0){
				write(STDOUT_FILENO, "list\nkill IP/PID\n", strlen("list\nkill IP/PID\n"));
			}
			else{
				write(STDOUT_FILENO, "Invalid command, type help to see a list of commands\n", strlen("Invalid command, type help to see a list of commands\n"));
			}
		}
		else{
			write(STDOUT_FILENO, "Command too big, retry", 22);
		}
	}
}

int main()
{
	signal(SIGCHLD, client_term);
	ppid = getpid();
	//keyboard reader only allowed for parent process
	pthread_create(&keyb_tid,NULL,&keyboard_reader,&ppid);
	int sock, length;
	struct sockaddr_in server;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = 0;
	if (bind(sock, (struct sockaddr *) &server, sizeof(server))) {
		perror("binding stream socket");
		exit(1);
	}

	length = sizeof(server);
	if (getsockname(sock, (struct sockaddr *) &server, (socklen_t*) &length)) {
		perror("getting socket name");
		exit(1);
	}
	printf("Socket has port #%d\n", ntohs(server.sin_port));
	fflush(stdout);

	listen(sock, 5);
	do {
		struct sockaddr_in client;
		socklen_t c_size = sizeof(struct sockaddr_in);
		msgsock = accept(sock, (struct sockaddr *)&client, &c_size);
		if (msgsock == -1){
			perror("accept");
			continue;
		}
		else{
			inet_ntop(AF_INET, &client, c_addr[ps_in], INET_ADDRSTRLEN);
			strcpy(activity[ps_in], "ACTIVE");
			sock_list[ps_in] = msgsock;
			ch_pid[ps_in] = fork();
		}
		if(ch_pid[ps_in]>0){
			write(STDOUT_FILENO, "Client ", 7);
			write(STDOUT_FILENO, c_addr[ps_in], strlen(c_addr[ps_in]));
			write(STDOUT_FILENO, " connected\n", strlen(" connected\n"));
			ps_in++;
		}
		else do {
			signal(SIGCHLD, process_term);
			signal(SIGTERM, close_sock);
			bzero(buf, sizeof(buf));
			if ((rval = read(msgsock, buf, 1024)) < 0)
				perror("reading stream message");
			if (rval == 0){
				write(STDOUT_FILENO, "Ending Client ", strlen("Ending Client "));
				write(STDOUT_FILENO, c_addr[ps_in], strlen(c_addr[ps_in]));
				write(STDOUT_FILENO, "'s connection\n", strlen("'s connection\n"));
				exit(0);
			}
			else{
				write(STDOUT_FILENO, "Client ", 7);
				write(STDOUT_FILENO, c_addr[ps_in], strlen(c_addr[ps_in]));
				write(STDOUT_FILENO, " Sent==> ", strlen(" Sent==> "));
				if(write(STDOUT_FILENO, buf, rval) < 0)
					perror("print client message to screen");
				write(STDOUT_FILENO, "\n", 1);
				char* token = strtok(buf, " ");
   	 			int i = 0;

				while(token != NULL){
					arr[i++] = token;
					token = strtok(NULL, " ");
				}
				arr[i] = "@";
				
				if(strcmp("add", arr[0])==0 || strcmp("ADD", arr[0])==0){
					if(strcmp(arr[1],"@")==0 || strcmp(arr[2],"@")==0){
						write(msgsock, "Write atleast two numbers\n", strlen("Write atleast two numbers\n"));
						continue;
					}
					int in = 1;
        				float result = 0;
        				for(;in<i;in++){
            					float num;
            					sscanf(arr[in], "%f", &num);
            					result += num;
        				}
	        			char output[50];
	        			int size = sprintf(output, "%.2f", result);
	        			output[size] = '\n';
					write(msgsock, "The result is: ", 15);
	        			write(msgsock, output, size+1);
					write(STDOUT_FILENO, "Add command processed\n", strlen("Add command processed\n"));
    				}
				else if(strcmp(arr[0],"SUB")==0 || strcmp(arr[0],"sub")==0){
        				if(strcmp(arr[1],"@")==0 || strcmp(arr[2],"@")==0){
						write(msgsock, "Write atleast two numbers\n", strlen("Write atleast two numbers\n"));
						continue;
					}
					int in = 2;
        				float result = 0;
					sscanf(arr[1], "%f", &result);
        				for(;in<i;in++){
            					float num;
            					sscanf(arr[in], "%f", &num);
            					result -= num;
        				}
	        			char output[50];
	        			int size = sprintf(output, "%.2f", result);
	        			output[size] = '\n';
					write(msgsock, "The result is: ", 15);
	        			write(msgsock, output, size+1);
					write(STDOUT_FILENO, "Sub command processed\n", strlen("Sub command processed\n"));
    				}
    				else if(strcmp(arr[0],"MUL")==0 || strcmp(arr[0],"mul")==0){
					if(strcmp(arr[1],"@")==0 || strcmp(arr[2],"@")==0){
						write(msgsock, "Write atleast two numbers\n", strlen("Write atleast two numbers\n"));
						continue;
					}
					int in = 1;
        				float result = 1;
        				for(;in<i;in++){
            					float num;
            					sscanf(arr[in], "%f", &num);
            					result *= num;
        				}
	        			char output[50];
	        			int size = sprintf(output, "%.2f", result);
	        			output[size] = '\n';
					write(msgsock, "The result is: ", 15);
	        			write(msgsock, output, size+1);
					write(STDOUT_FILENO, "Mul command processed\n", strlen("Mul command processed\n"));
				}
    				else if(strcmp(arr[0],"DIV")==0 || strcmp(arr[0],"div")==0){
					if(strcmp(arr[1],"@")==0 || strcmp(arr[2],"@")==0){
						write(msgsock, "Write atleast two numbers\n", strlen("Write atleast two numbers\n"));
						continue;
					}
					int in = 2;
        				float result = 0;
					sscanf(arr[1], "%f", &result);
        				for(;in<i;in++){
						float num;
            					sscanf(arr[in], "%f", &num);
            					result /= num;
        				}
	        			char output[50];
	        			int size = sprintf(output, "%.2f", result);
	        			output[size] = '\n';
					write(msgsock, "The result is: ", 15);
	        			write(msgsock, output, size+1);
					write(STDOUT_FILENO, "Mul command processed\n", strlen("Mul command processed\n"));
    				}
    				else if(strcmp("run", arr[0])==0 || strcmp("RUN", arr[0])==0){
					if(strcmp(arr[1], "@")==0){
						write(msgsock, "Write a process name\n", strlen("Write a process name\n"));
						continue;
					}        				
					char* args[i];
        				int in = 2; int index = 0;
        				for(;in<i;in++){
            					args[index++] = arr[in];
        				}
        				args[index] = NULL;

        				int p = fork();
        				if(p==0){
						signal(SIGTERM, SIG_DFL);
            					if(execv(arr[1], args)==-1){
               						if(execvp(arr[1], args)==-1){
								perror("Exec Failure");
								write(msgsock, "Exec Failed\n", 12);
								exit(2);
							}
            					}
        				}
					else if(p>0){
						signal(SIGCHLD, SIG_DFL);
						usleep(125000);
						int ret = 0;
						waitpid(p, &ret, WNOHANG);
						ret = WEXITSTATUS(ret);
						if(ret!=2){
							write(msgsock, "Exec Successful\n", strlen("Exec Successful\n"));
							process_id[process_counter] = p;
							strcpy(process_name[process_counter], arr[1]);
							strcpy(process_status[process_counter], "ACTIVE");					
							process_start[process_counter] = time(NULL);
							process_end[process_counter] = (time_t)0;
							process_counter++;
						}
						signal(SIGCHLD, process_term);
					}
    				}
				else if(strcmp("list", arr[0])==0 || strcmp("LIST", arr[0])==0){
					write(msgsock, "Process_ID\tProcess_name\tActive_Status\tStart_Time\t\t\tEnd_time\n", strlen("Process_ID\tProcess_name\tActive_Status\tStart_Time\t\t\tEnd_time\n"));
					int i = 0;
					while(i<process_counter){
						char str[50];
						sprintf(str, "%d", process_id[i]);
						write(msgsock, str, strlen(str));
						write(msgsock, "\t\t", 2);
						write(msgsock, process_name[i], strlen(process_name[i]));
						write(msgsock, "\t\t", 2);
						write(msgsock, process_status[i], strlen(process_status[i]));
						write(msgsock, "\t\t", 2);
						time_t result = process_start[i];
						write(msgsock, asctime(gmtime(&result)), strlen(asctime(gmtime(&result)))-1);
						write(msgsock, "\t", 1);
						time_t result1 = process_end[i];
						write(msgsock, asctime(gmtime(&result1)), strlen(asctime(gmtime(&result1)))-1);
						write(msgsock, "\n", 1);
						i++;
					}
				}
				else if(strcmp("kill", arr[0])==0 || strcmp("KILL", arr[0])==0){
					int num; int option = 0;
					int ret = sscanf(arr[1], "%d", &num);
					if(ret==1){
						int i = 0;
						for(;i<process_counter;i++){
							if(process_id[i]==num)
							break;
						}
						if(kill(process_id[i], SIGTERM)<0)
							perror("Kill");
						else{
							write(STDOUT_FILENO, "Process Terminated\n", strlen("Process Terminated\n"));
						}
					}
					else{
						int i = 0;
						for(;i<process_counter;i++){
							if((strcmp(process_name[i], arr[1])==0)&&(strcmp(process_status[i], "ACTIVE")==0))
							break;
						}
						if(kill(process_id[i], SIGTERM)<0)
							perror("Kill");
						else{
							write(STDOUT_FILENO, "Process Terminated\n", strlen("Process Terminated\n"));
						}
					}
				}
				else if(strcmp("print", arr[0])==0 || strcmp("PRINT", arr[0])==0){
					if(strcmp("ip", arr[1])==0 || strcmp("IP", arr[1])==0){
						write(msgsock, "Your IP is: ", strlen("Your IP is: "));
						write(msgsock, c_addr[ps_in], strlen(c_addr[ps_in]));
						write(msgsock, "\n", 1);
					}
				}
				else if(strcmp("help", arr[0])==0 || strcmp("HELP", arr[0])==0){
					write(msgsock, "Add\nSub\nMul\nDiv\nrun process_name\nlist\nkill pid/name\nprint ip\n", strlen("Add\nSub\nMul\nDiv\nrun process_name\nlist\nkill pid/name\nprint ip\n"));
				}
				else{
					write(msgsock, "Invalid command, type help to see a list of commands\n", strlen("Invalid command, type help to see a list of commands\n"));
				}
			}
		} while (rval != 0);
		close(msgsock);
	} while (1);
}
