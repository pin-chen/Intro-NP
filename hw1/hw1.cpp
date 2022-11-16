#include <bits/stdc++.h>

#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
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
	bool registed = false;
	bool nick = false;
	bool user = false;
	string cmd;
};

struct channelInfo{
	string topic;
	set<string> users;
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

void print(int i, string out){
	send(client[i], out.c_str(), out.size(), MSG_NOSIGNAL);
}

void response(int i, int code, vector<string> args){
	string prefix;
	if(code == 1){
		prefix = ":mircd 001 ";
	}else{
		prefix = ":mircd " + to_string(code) + " ";
	}
	string suffix = "\r\n";
	string nickname = usersData[i].nickname + " ";
	switch(code){
        case RPL_LISTSTART:{
			print(i, prefix + nickname + "Channel :Users Name" + suffix);
            break;
        }
        case RPL_LIST:{
			print(i, prefix + nickname + args[0] + " " + to_string(channelData[args[0]].users.size()) + " :" + channelData[args[0]].topic + suffix);
            break;
        }
        case RPL_LISTEND:{
			print(i, prefix + nickname + ":End of /LIST" + suffix);
            break;
        }
        case RPL_NOTOPIC:{
			print(i, prefix + nickname + args[0] + " :No topic is set" + suffix);
            break;
        }
        case RPL_TOPIC:{
			print(i, prefix + nickname + args[0] + " :" + args[1] + suffix);
            break;
        }
        case RPL_NAMREPLY:{
			string users = "";
			for(auto user : channelData[args[0]].users){
				users += user + " ";
			}
			print(i, prefix + nickname + args[0] + " :" + users + suffix);
            break;
        }
        case RPL_ENDOFNAMES:{
			print(i, prefix + nickname + args[0] + " :End of /NAMES list" + suffix);
            break;
        }
        case RPL_MOTD:{
			print(i, prefix + nickname + ":-  Hello, World!" + suffix);
			print(i, prefix + nickname + ":-               @                    _ " + suffix);
			print(i, prefix + nickname + ":-   ____  ___   _   _ _   ____.     | |" + suffix);
			print(i, prefix + nickname + ":-  /  _ `'_  \\ | | | '_/ /  __|  ___| |" + suffix);
			print(i, prefix + nickname + ":-  | | | | | | | | | |   | |    /  _  |" + suffix);
			print(i, prefix + nickname + ":-  | | | | | | | | | |   | |__  | |_| |" + suffix);
			print(i, prefix + nickname + ":-  |_| |_| |_| |_| |_|   \\____| \\___,_|" + suffix);
			print(i, prefix + nickname + ":-  minimized internet relay chat daemon" + suffix);
			print(i, prefix + nickname + ":-" + suffix);
            break;
        }
        case RPL_MOTDSTART:{
			print(i, prefix + nickname + ":- mircd Message of the day -" + suffix);
            break;
        }
        case RPL_ENDOFMOTD:{
			print(i, prefix + nickname + ":End of /MOTD command" + suffix);
            break;
        }
        case RPL_USERSSTART:{
			print(i, prefix + nickname + ":UserID   Terminal  Host" + suffix);
            break;
        }
        case RPL_USERS:{
			char str[50] = {};
			sprintf(str, ":%-8s %-9s %-8s", args[0].c_str(), "-", args[1].c_str());
			print(i, prefix + nickname + string(str) + suffix);
            break;
        }
        case RPL_ENDOFUSERS:{
			print(i, prefix + nickname + ":End of users" + suffix);
            break;
        }
        case ERR_NOSUCHNICK:{
			print(i, prefix + nickname + args[0] + " :No such nick/channel" + suffix);
            break;
        }
        case ERR_NOSUCHCHANNEL:{
			print(i, prefix + nickname + args[0] + " :No such channel" + suffix);
            break;
        }
        case ERR_NORECIPIENT:{
			print(i, prefix + nickname + ":No recipient given (" + args[0] + ")" + suffix);
            break;
        }
        case ERR_NOTEXTTOSEND:{
			print(i, prefix + nickname + ":No text to send" + suffix);
            break;
        }
        case ERR_UNKNOWNCOMMAND:{
			print(i, prefix + nickname + args[0] + " :Unknown command" + suffix);
            break;
        }
        case ERR_NONICKNAMEGIVEN:{
			print(i, prefix + ":No nickname given" + suffix);
            break;
        }
        case ERR_NICKCOLLISION:{
			print(i, prefix + args[0] + " :Nickname collision KILL" + suffix);
            break;
        }
        case ERR_NOTONCHANNEL:{
			print(i, prefix + nickname + args[0] + " :You're not on that channel" + suffix);
            break;
        }
        case ERR_NEEDMOREPARAMS:{
			print(i, prefix + nickname + args[0] + " :Not enough parameters" + suffix);
            break;
        }
        case ERR_NOTREGISTERED:{
			print(i, prefix + ":You have not registered" + suffix);
            break;
        }
        case ERR_NOORIGIN:{
			print(i, prefix + nickname + ":No origin specified" + suffix);
            break;
        }
        case RPL_WELCOME:{
			print(i, prefix + nickname + ":Welcome to the minimized IRC daemon!" + suffix);
            break;
        }
        case RPL_LUSERCLIENT:{
			print(i, prefix + nickname + ":There are " + to_string(numUsers) + " users and 0 invisible on 1 servers" + suffix);
            break;
        }
        case RPL_NOUSERS:{
			print(i, prefix + nickname + ":Nobody logged in" + suffix);
            break;
        }
	}
}

void excute(int i, vector<string>& cmd){
	if(cmd[0] == "NICK"){
		if(cmd.size() < 2){
			response(i, ERR_NONICKNAMEGIVEN, {});
		}else if(userConnection.find(cmd[1]) != userConnection.end()){
			response(i, ERR_NICKCOLLISION, {cmd[1]});
		}else{
			if(usersData[i].nick)
				for(auto j : userConnection)
					print(j.s, ":" + usersData[i].nickname + " NICK " + cmd[1] + "\r\n");
			usersData[i].nickname = cmd[1];
			usersData[i].nick = true;
			userConnection[usersData[i].nickname] = i;
		}
	}else if(cmd[0] == "USER"){
		if(cmd.size() < 5){
			response(i, ERR_NEEDMOREPARAMS, {"USER"});
		}else{
			usersData[i].username = cmd[1];
			usersData[i].hostname = cmd[2];
			usersData[i].servername = cmd[3];
			string realname = cmd[4];
			for(int index = 5; index < cmd.size(); index++){
				realname += " " + cmd[index];
			}
			usersData[i].realname = realname.substr(1);
			usersData[i].user = true;
		}
	}else if(!usersData[i].registed){
		response(i, ERR_NOTREGISTERED, {});
		return;
	}else if(cmd[0] == "PING"){
		if(cmd.size() < 2){
			response(i, ERR_NOORIGIN, {});
		}else{
			print(i, "PONG :" + cmd[1] + "\r\n");
		}
	}else if(cmd[0] == "LIST"){
		response(i, RPL_LISTSTART, {});
		if(cmd.size() < 2){
			for(auto channel : channelData){
				response(i, RPL_LIST, {channel.f});
			}
		}else{
			response(i, RPL_LIST, {cmd[1]});
		}
		response(i, RPL_LISTEND, {});
	}else if(cmd[0] == "JOIN"){
		if(cmd.size() < 2){
			response(i, ERR_NEEDMOREPARAMS, {"JOIN"});
		}else if(cmd[1][0] != '#'){
			response(i, ERR_NOSUCHCHANNEL, {cmd[1]});
		}else{
			channelData[cmd[1]].users.emplace(usersData[i].nickname);
			for(auto user : channelData[cmd[1]].users)
				print(userConnection[user], ":" + usersData[i].nickname + " " + cmd[0] + " :" + cmd[1] + "\r\n");
			if(channelData[cmd[1]].topic == ""){
				response(i, RPL_NOTOPIC, {cmd[1]});
			}else{
				response(i, RPL_TOPIC, {cmd[1], channelData[cmd[1]].topic});
			}
			response(i, RPL_NAMREPLY, {cmd[1]});
			response(i, RPL_ENDOFNAMES, {cmd[1]});
		}
	}else if(cmd[0] == "TOPIC"){
		if(cmd.size() < 2){
			response(i, ERR_NEEDMOREPARAMS, {"TOPIC"});
		}else if(channelData[cmd[1]].users.find(usersData[i].nickname) == channelData[cmd[1]].users.end()){
			response(i, ERR_NOTONCHANNEL, {cmd[1]});
		}else if(cmd.size() > 2){
			string topic = cmd[2];
			for(int index = 3; index < cmd.size(); index++){
				topic += " " + cmd[index];
			}
			channelData[cmd[1]].topic = topic.substr(1);
			response(i, RPL_TOPIC, {cmd[1], channelData[cmd[1]].topic});
		}else if(channelData[cmd[1]].topic == ""){
			response(i, RPL_NOTOPIC, {cmd[1]});
		}else{
			response(i, RPL_TOPIC, {cmd[1], channelData[cmd[1]].topic});
		}
	}else if(cmd[0] == "NAMES"){
		if(cmd.size() < 2){
			for(auto channel : channelData){
				response(i, RPL_NAMREPLY, {channel.f});
				response(i, RPL_ENDOFNAMES, {channel.f});
			}
		}else{
			response(i, RPL_NAMREPLY, {cmd[1]});
			response(i, RPL_ENDOFNAMES, {cmd[1]});
		}
	}else if(cmd[0] == "PART"){
		if(cmd.size() < 2){
			response(i, ERR_NEEDMOREPARAMS, {"PART"});
		}else if(channelData.find(cmd[1]) == channelData.end()){
			response(i, ERR_NOSUCHCHANNEL, {cmd[1]});
		}else if(channelData[cmd[1]].users.find(usersData[i].nickname) == channelData[cmd[1]].users.end()){
			response(i, ERR_NOTONCHANNEL, {cmd[1]});
		}else{
			for(auto user : channelData[cmd[1]].users)
				print(userConnection[user], ":" + usersData[i].nickname + " " + cmd[0] + " :" + cmd[1] + "\r\n");
			channelData[cmd[1]].users.erase(usersData[i].nickname);
		}
	}else if(cmd[0] == "USERS"){
		response(i, RPL_USERSSTART, {});
		if(userConnection.size() == 0){
			response(i, RPL_NOUSERS, {});
		}else{
			for(auto user : userConnection){
				response(i, RPL_USERS, {user.f, string(usersData[user.s].ip4)});
			}
		}
		response(i, RPL_ENDOFUSERS, {});
	}else if(cmd[0] == "PRIVMSG"){
		if(cmd.size() < 2){
			response(i, ERR_NORECIPIENT, {"PRIVMSG"});
		}else if(cmd.size() < 3){
			response(i, ERR_NOTEXTTOSEND, {});
		}else if(channelData.find(cmd[1]) == channelData.end()){
			response(i, ERR_NOSUCHNICK, {cmd[1]});
		}else{
			for(auto user : channelData[cmd[1]].users){
				if(userConnection[user] == i) continue;
				string msg = cmd[2];
				for(int index = 3; index < cmd.size(); index++){
					msg += " " + cmd[index];
				}
				print(userConnection[user], ":" + usersData[i].nickname + " PRIVMSG " + cmd[1] + " " + msg + "\r\n");
			}
		}
	}else{
		response(i, ERR_UNKNOWNCOMMAND, {cmd[0]});
	}
	if(!usersData[i].registed){
		if(usersData[i].nick && usersData[i].user){
			response(i, RPL_WELCOME, {});
			response(i, RPL_LUSERCLIENT, {});
			response(i, RPL_MOTDSTART, {});
			response(i, RPL_MOTD, {});
			response(i, RPL_ENDOFMOTD, {});
			usersData[i].registed = true;
		}
	}
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
	int opt;
	int val, len = sizeof(val);
	int e = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &val, len);
	if(e){
		perror("sockopt");
	}
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
					userConnection.erase(usersData[i].nickname);
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
						//IFQUIT
						if(cmd[0] == "QUIT"){
							close(sockfd);
							FD_CLR(sockfd, &allset);
							client[i] = -1;
							//QUIT
							userConnection.erase(usersData[i].nickname);
							usersData.erase(i);
							numUsers--;
							break;
						}
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