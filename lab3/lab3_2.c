#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <arpa/inet.h> // inet_addr()
#define SA struct sockaddr
char MB[5120];
char MB2[2560];
char ans[2000];
void func(int sockfd, char *t){
	int i = 0;
	for(; i<1000; i++){
		if(strcmp(t, "1") == 0){
			printf("a");
			write(sockfd, MB, sizeof(MB));
			
			usleep(5000);
		}else if(strcmp(t, "1.5") == 0){
			printf("b");
			write(sockfd, MB, sizeof(MB));
			write(sockfd, MB2, sizeof(MB2));
			usleep(5000);
		}else if(strcmp(t, "2") == 0){
			printf("c");
			write(sockfd, MB, sizeof(MB));
			write(sockfd, MB, sizeof(MB));
			usleep(5000);
		}else if(strcmp(t, "3") == 0){
			printf("d");
			write(sockfd, MB, sizeof(MB));
			write(sockfd, MB, sizeof(MB));
			write(sockfd, MB, sizeof(MB));
			usleep(5000);
		}
	}
}
 
int main(int argc, char**argv){
	memset(MB, 'A', sizeof(MB));
	memset(MB2, 'A', sizeof(MB2));
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket Error\n");
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("140.113.213.213");
    servaddr.sin_port = htons(10003);
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Connect Error\n");
        exit(1);
    }
	printf("%s\n\n", argv[1]);
    func(sockfd, argv[1]);
    close(sockfd);
}