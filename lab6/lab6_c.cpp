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
#define SA struct sockaddr
#define numData 10
int sockfd;
int dataSock[numData];
char MB[1000000];
char MB2[100]; 
void handler(int signo){
	write(sockfd, "/report\n", 8);
	read(sockfd, MB2, sizeof(MB2));
	printf("%s", MB2);
	for(int i = 0; i < numData; i++){
		close(dataSock[i]);
	}
	//memset(MB2, '\0', sizeof(MB2));
	//write(sockfd, "/clients\n", 9);
	//read(sockfd, MB2, sizeof(MB2));
	//printf("%s", MB2);
	//memset(MB2, '\0', sizeof(MB2));
	close(sockfd);
	sleep(2);
	exit(0);
}

int main(int argc, char**argv){
	signal(SIGTERM, handler);
	signal(SIGINT, handler);
	memset(MB2, '\0', sizeof(MB2));
    struct sockaddr_in servaddr, servaddr2;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket Error\n");
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(9998);
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Connect Error\n");
        exit(1);
    }
	
    bzero(&servaddr2, sizeof(servaddr2));
    servaddr2.sin_family = AF_INET;
    servaddr2.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr2.sin_port = htons(9999);
	for(int i = 0; i < numData; i++){
		dataSock[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (dataSock[i] == -1) {
			printf("Socket Error\n");
			exit(1);
		}
		if (connect(dataSock[i], (SA*)&servaddr2, sizeof(servaddr2)) != 0) {
			printf("Connect Error\n");
			exit(1);
		}

	}
	
	write(sockfd, "/reset\n", 7);
	read(sockfd, MB2, sizeof(MB2));
	printf("%s", MB2);
	memset(MB2, '\0', sizeof(MB2));
	
	//write(sockfd, "/clients\n", 9);
	//read(sockfd, MB2, sizeof(MB2));
	//printf("%s", MB2);
	//memset(MB2, '\0', sizeof(MB2));
	while(1){
		for(int i = 0; i < numData; i++) send(dataSock[i], MB, sizeof(MB), MSG_DONTWAIT & MSG_NOSIGNAL);	
	}
}
