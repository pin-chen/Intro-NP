#include <bits/stdc++.h>
#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h> 	// bzero()
#include <unistd.h> 	// read(), write(), close()
#include <arpa/inet.h> 	// inet_addr()
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#define SA struct sockaddr
#define	MAXLINE	4096
#define fd first
#define s second
using namespace std;
long long int counter = 0;
double globalTime;

struct usersInfo{
	char ip4[INET_ADDRSTRLEN];
	int port;
	string msg;
	bool isDataSink = false;
};

int numDataSink = 0;
int	client[FD_SETSIZE];
map<int, usersInfo> usersData;

void handler(int signo){
	printf("\nClose\n");
	exit(0);
}

double getTime(){
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return (double) tv.tv_sec +  1e-6 * tv.tv_usec;
}

void print(int fd, string out){
	send(fd, out.c_str(), out.size(), MSG_NOSIGNAL);
}
 
int main(int argc, char**argv){
	signal(SIGINT, handler);
	globalTime = getTime();
	
	int 				i, j, connfd, sockfd, listenfd, listenfd2, maxi, maxfd, nready, port = 9998;
	fd_set				rset, wset, eset, allset;
	socklen_t 			clilen;
	ssize_t				n;
	char				buf[MAXLINE];
	struct sockaddr_in 	servaddr, cliaddr;
	
	sscanf(argv[1], "%d", &port);
	
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bind(listenfd, (SA*) &servaddr, sizeof(servaddr));
	listen(listenfd, 1010);
	
	listenfd2 = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port+1);
    bind(listenfd2, (SA*) &servaddr, sizeof(servaddr));
	listen(listenfd2, 1009);
	
	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++) client[i] = -1;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	FD_SET(listenfd2, &allset);
	for ( ; ; ) {
		rset = allset;
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);
		if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (SA *) &cliaddr, &clilen);
			for (i = 0; i < FD_SETSIZE; i++){
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			}
			if (i == FD_SETSIZE){
				perror("too many clients");
				exit(1);
			}
			inet_ntop(AF_INET, &(cliaddr.sin_addr), usersData[i].ip4, INET_ADDRSTRLEN);
			usersData[i].port = ntohs(cliaddr.sin_port);
			FD_SET(connfd, &allset);					/* add new descriptor to set */
			if (connfd > maxfd)	maxfd = connfd;			/* for select */
			if (i > maxi) 		maxi = i;				/* max index in client[] array */
			if (--nready <= 0) 	continue;				/* no more readable descriptors */
		}else if (FD_ISSET(listenfd2, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd2, (SA *) &cliaddr, &clilen);
			for (i = 0; i < FD_SETSIZE; i++){
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			}
			if (i == FD_SETSIZE){
				perror("too many clients");
				exit(1);
			}
			numDataSink++;
			inet_ntop(AF_INET, &(cliaddr.sin_addr), usersData[i].ip4, INET_ADDRSTRLEN);
			usersData[i].port = ntohs(cliaddr.sin_port);
			usersData[i].isDataSink = true;
			FD_SET(connfd, &allset);					/* add new descriptor to set */
			if (connfd > maxfd)	maxfd = connfd;			/* for select */
			if (i > maxi) 		maxi = i;				/* max index in client[] array */
			if (--nready <= 0) 	continue;				/* no more readable descriptors */
		}

		for(i = 0; i <= maxi; i++){	/* check all clients for data */
			if( (sockfd = client[i]) < 0) continue;
			if(FD_ISSET(sockfd, &rset)){
				if( (n = read(sockfd, buf, MAXLINE)) == 0){	/*4connection closed by client */
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					if(usersData[i].isDataSink) numDataSink--;
					usersData.erase(i);
				}else{
					if(usersData[i].isDataSink){
						counter += n;
					}else{
						for(j = 0; j < n; j++){
							if(buf[j] != '\n'){
								usersData[i].msg.push_back(buf[j]);
								continue;
							}
							string msg = usersData[i].msg;
							usersData[i].msg.clear();
							if(msg[0] == '/'){
								if(msg.substr(1, 5) == "reset"){
									globalTime = getTime();
									print(client[i], to_string(globalTime) + " RESET " + to_string(counter) + "\n");
									counter = 0;
								}else if(msg.substr(1, 4) == "ping"){
									double time = getTime();
									print(client[i], to_string(time) + " PONG\n");
								}else if(msg.substr(1, 6) == "report"){
									double time = getTime();
									double elapsedTime = time - globalTime;
									double mbps = 8.0 * counter / 1000000.0 / elapsedTime;
									print(client[i], to_string(time) + " REPORT " + to_string(counter) + " " + to_string(elapsedTime) + "s " + to_string(mbps) + "Mbps\n" );
								}else if(msg.substr(1, 7) == "clients"){
									double time = getTime();
									print(client[i], to_string(time) + " CLIENTS " + to_string(numDataSink)+ "\n");
								}else{
									print(client[i], "Error!");
								}
							}
						}
					}
				}
				if(--nready <= 0) break; /* no more readable descriptors */
			}
		}
	}
	return 0;
}