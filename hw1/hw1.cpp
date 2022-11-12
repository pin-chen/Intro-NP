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

#include "response.h"

#define SA struct sockaddr
#define	MAXLINE	4096
#define f first
#define s second

using namespace std;

struct usersInfo{
	char ip4[INET_ADDRSTRLEN];
	int port;
	string nickname;
	string username;
	string hostname;
	string servername;
	string realname;
	string channel;
	bool nick = false;
	bool user = false;
	string cmd;
};

struct channelInfo{
	string topic;
	vector<string> users;
};

int numUsers = 0;
int	client[FD_SETSIZE];
map<int, usersInfo> usersData;
map<string, int> userConnection;
map<string, channelInfo> channelData;

void handler(int signo){
	printf("\nClose\n");
	exit(0);
}

void splitArgument(vector<string> &dst, string &src){
	dst.clear();
	string arg;
	int begin = 0, charType = 0; //0 otherwise 1 space
	for(; begin < src.size(); begin++){
		if(src[begin] == ' ' || src[begin] == '\r'){
			if(charType == 1){
				dst.push_back(arg);
				arg.clear();
				charType = 0;
			}
		}else{
			if(charType == 0) charType = 1;
			arg.push_back(src[begin]);
		}
	}
	if(arg.size() != 0){
		dst.push_back(arg);
	} 
}

void print(int i, int code, string src){
	string out = ":mircd " + to_string(code) + " " + src + "\r\n";
	send(client[i], out.c_str(), out.size(), MSG_NOSIGNAL);
}

void broadcast(int i, int code, string& src){
	for(auto userInfo : usersData){
		if(userInfo.f == i) continue;
		print(client[userInfo.f], code, src);
	}
}

void excute(int i, vector<string>& cmd){
	
}

int main(int argc, char**argv){
	signal(SIGINT, handler);
	int 				i, j, connfd, sockfd, listenfd, maxi, maxfd, nready, port = 10004;
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
		fflush(stdout);
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
			//CONNECT
			numUsers++;
			inet_ntop(AF_INET, &(cliaddr.sin_addr), usersData[i].ip4, INET_ADDRSTRLEN);
			usersData[i].port = ntohs(cliaddr.sin_port);
			
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
					//QUIT
					usersData.erase(i);
					numUsers--;
				}else{
					for(j = 0; j < n; j++){
						if(buf[j] != '\n'){
							usersData[i].cmd.push_back(buf[j]);
							continue;
						}
						//RECV
						cout << usersData[i].cmd << '\n';
						//
						vector<string> cmd;
						splitArgument(cmd, usersData[i].cmd);
						usersData[i].cmd.clear();
						if(cmd.size() <= 0) continue;
						//EXCUTE
						excute(i, cmd);
					}
				}
				if(--nready <= 0) break; /* no more readable descriptors */
			}
		}
	}
	return 0;
}