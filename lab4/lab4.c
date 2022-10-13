#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <arpa/inet.h> // inet_addr()
#include <time.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#define SA struct sockaddr
 int listenfd;
void handler(int signo){
	printf("\nClose\n");
	close(listenfd);
	exit(0);
}
 
int main(int argc, char**argv){
	signal(SIGINT, handler);
	if(argc < 3){
		printf("Error Argu.\n");
		exit(1);
	}
	int port = 11111;
	sscanf(argv[1], "%d", &port);
    int connfd, savefd;
	socklen_t child;
    struct sockaddr_in servaddr, cliaddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bind(listenfd, (SA*) &servaddr, sizeof(servaddr));
	listen(listenfd, 10);
	pid_t pid;
	while(1){
		connfd = accept(listenfd, (SA *)&cliaddr, &child);
		char ip4[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(cliaddr.sin_addr), ip4, INET_ADDRSTRLEN);
        if ((pid = fork()) == -1){
            close(connfd);
            continue;
        }else if(pid > 0){
			printf("New connection from %s:%d\n", ip4, child);
			close(connfd);
			int stat;
			while ( waitpid(-1, &stat, WNOHANG) > 0);
			continue;
        }else if(pid == 0){
			dup2(connfd, 0);
			dup2(connfd, 1);
			close(connfd);
            int n = execvp(argv[2], &argv[2]);
			if(n == -1) {
				perror("Error!!!!!!:");
				exit(1);
			}
			exit(0);
        }
	}
    close(listenfd);
	return 0;
}