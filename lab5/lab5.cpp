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

struct usersInfo{
	char ip4[INET_ADDRSTRLEN];
	int port;
	string name;
	string msg;
};

int name = 0;
int numUsers = 0;
int	client[FD_SETSIZE];
map<int, usersInfo> usersData;

void handler(int signo){
	printf("\nClose\n");
	exit(0);
}

void print(int fd, string out){
	send(fd, out.c_str(), out.size(), MSG_NOSIGNAL);
}

void printTime(int fd){
	time_t nowTime = time(NULL);
	char strNowTime[50];
	memset(strNowTime, '\0', 50);
	strftime(strNowTime, sizeof(strNowTime), "%Y-%m-%d %H:%M:%S", localtime(&nowTime));
	print(fd, string(strNowTime));
}

void broadcast(string out, int i){
	for(auto userInfo : usersData){
		if(userInfo.fd == i) continue;
		printTime(client[userInfo.fd]);
		print(client[userInfo.fd], out + "\n");
	}
}
 
int main(int argc, char**argv){
	signal(SIGINT, handler);
	
	int 				i, j, connfd, sockfd, listenfd, maxi, maxfd, nready, port = 10005;
	fd_set				rset, wset, eset, allset;
	socklen_t 			clilen;
	ssize_t				n;
	char				buf[MAXLINE];
	struct sockaddr_in 	servaddr, cliaddr;
	struct timeval 		tv;
	
	tv.tv_sec = 10;
	tv.tv_usec = 1000;
	sscanf(argv[1], "%d", &port);
	
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    bind(listenfd, (SA*) &servaddr, sizeof(servaddr));
	listen(listenfd, 1010);
	
	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++) client[i] = -1;
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	
	for ( ; ; ) {
		rset = allset;
		nready = select(maxfd+1, &rset, NULL, NULL, &tv);
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
			
			name++;
			numUsers++;
			inet_ntop(AF_INET, &(cliaddr.sin_addr), usersData[i].ip4, INET_ADDRSTRLEN);
			usersData[i].port = ntohs(cliaddr.sin_port);
			printf("* client connected from %s:%d\n", usersData[i].ip4, usersData[i].port);
			printTime(connfd);
			print(connfd, " *** Welcome to the simple CHAT server\n");
			printTime(connfd);
			print(connfd, " *** Total " + to_string(numUsers) + " users online now. Your name is <");
			usersData[i].name = to_string(name);
			print(connfd, usersData[i].name + ">\n");
			broadcast(" *** User <" + usersData[i].name + "> has just landed on the server", i);
	
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
					printf("* client %s:%d disconnected\n", usersData[i].ip4,  usersData[i].port);
					broadcast(" *** User <" + usersData[i].name + "> has left the server", i);
					usersData.erase(i);
					numUsers--;
				}else{
					for(j = 0; j < n; j++){
						if(buf[j] != '\n'){
							usersData[i].msg.push_back(buf[j]);
							continue;
						}
						string msg = usersData[i].msg;
						usersData[i].msg.clear();
						if(msg[0] == '/'){
							if(msg.substr(1, 5) == "name " && msg.size() > 7){
								string oldName = usersData[i].name;
								usersData[i].name = msg.substr(6);
								printTime(sockfd);
								print(sockfd, " *** Nickname changed to <" + usersData[i].name + ">\n");
								broadcast(" *** User <" + oldName + "> renamed to <" + usersData[i].name + ">", i);
							}else if(msg.substr(1, 3) == "who"){
								print(sockfd, "--------------------------------------------------\n");
								for(auto userInfo : usersData){
									if(userInfo.fd == i) 	print(sockfd, "* ");
									else 					print(sockfd, "  ");
									print(sockfd, userInfo.s.name + "\t" + string(userInfo.s.ip4) + ":" + to_string(userInfo.s.port) + "\n");
								}
								print(sockfd, "--------------------------------------------------\n");
							}else{
								printTime(sockfd);
								print(sockfd, " *** Unknown or incomplete command <" + msg + ">\n");
							}
						}else{
							broadcast(" <" + usersData[i].name + "> " + msg, i);
						}
					}
				}
				if(--nready <= 0) break; /* no more readable descriptors */
			}
		}
	}
	return 0;
}