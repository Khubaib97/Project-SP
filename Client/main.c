#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

char keyb_in[500];
pthread_t keyb_tid;
int sock;

void* keyboard_reader(void* args){
	while(1) {
		bzero(keyb_in, 500);
    		int readchars = read(STDIN_FILENO, keyb_in, 500);
    		if(readchars < 0)
        		perror("Reading from std input");
    		if(keyb_in[readchars-1]=='\n'){
			keyb_in[readchars-1] = '\0';
			if (write(sock, keyb_in, readchars) < 0)
				perror("Writing on stream socket");
		}
		else{
			write(STDOUT_FILENO, "Command too big, retry", 22);
		}
	}
}

int main(int argc, char *argv[])
	{
	struct sockaddr_in server;
	struct hostent *hp;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("opening stream socket");
		exit(1);
	}

	server.sin_family = AF_INET;
	hp = gethostbyname(argv[1]);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", argv[1]);
		exit(2);
	}
	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_port = htons(atoi(argv[2]));

	if (connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) {
		perror("connecting stream socket");
		exit(1);
	}
	write(STDOUT_FILENO, "Connected\n", 10);
    	pthread_create(&keyb_tid,NULL,&keyboard_reader,NULL);
	//read socket for response from server
	char msg[500];
	while(1){
		bzero(msg, 500);
		int ret = read(sock, msg, 500);
		if(ret<0)
			perror("Error receiving message");
		else if(ret>0){
			write(STDOUT_FILENO, msg, ret);
			if(strcmp(msg, "Server Terminated Connection")==0){
				write(STDOUT_FILENO, "\n", 1);				
				exit(0);
			}
		}
	}
	close(sock);
}
