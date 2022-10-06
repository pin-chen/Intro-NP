#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <arpa/inet.h> // inet_addr()
#include <netdb.h>
#define SA struct sockaddr
char buff[100000001];
char ans[2000];
void func(int sockfd){
    int n;
	bzero(buff, sizeof(buff));
	n = 0;
	read(sockfd, buff, sizeof(buff));
	bzero(buff, sizeof(buff));
	write(sockfd, "GO\n", 3);
	read(sockfd, buff, sizeof(buff));
	printf("%s", buff);
	bzero(buff, sizeof(buff));
	int i = 0;
	char c[1];
	while(1){
		bzero(c, sizeof(c));
		read(sockfd, c, sizeof(c));
		i++;
		if(c[0] == '='){
			break;
		}
	}
	read(sockfd, buff, sizeof(buff));
	printf("%s", buff);
	bzero(buff, sizeof(buff));
	sprintf(ans, "%d", i - 2); 
	printf("%s\n", ans);
	write(sockfd, ans, sizeof(ans));
	write(sockfd, "\n", 1);
	read(sockfd, buff, sizeof(buff));
	printf("%s", buff);
	bzero(buff, sizeof(buff));
}
 
int main(){
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
    servaddr.sin_port = htons(10002);
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
        printf("Connect Error\n");
        exit(1);
    }
    func(sockfd);
    close(sockfd);
}