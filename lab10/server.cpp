#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#define err_quit(m) { perror(m); exit(-1); }
#define MAXSIZE 1300
#define IPPROTO_XDP 161
using namespace std;

int numFiles;
string path, broadcastAddress;

map<string, vector<string>> dataBuf;
vector<string> packetBuf;

void ipheader(struct iphdr *ip, size_t len){
	ip->version  = 4;
	ip->ihl      = 5;
	ip->tos      = 16;
	ip->tot_len  = sizeof(struct iphdr) + len;
	ip->id       = htons(11111);
	ip->ttl      = 64;
	ip->protocol = 161;
	ip->check = 0;
	ip->saddr = inet_addr("192.168.0.1");
	ip->daddr = inet_addr(broadcastAddress.c_str());
}

void server(){
	cout << "s_server\n";
	
	int socketfd, val = 1, ec;
	char buffer[MAXSIZE];
	string initFile;
	struct iphdr *ip = (struct iphdr *) buffer;
	struct sockaddr_in sin, csin;
	socklen_t csinlen = sizeof(csin);
	
	for(int i = 0; i < 1000; i++){
		if(i < 10) 			initFile = "00" + to_string(i);
		else if(i < 100) 	initFile = "0" + to_string(i);
		else				initFile = to_string(i);
		dataBuf[initFile].resize(40);
	}
	
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(9997);
	sin.sin_addr.s_addr = inet_addr(broadcastAddress.c_str());
	
	socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_XDP);
	if(socketfd == -1) err_quit("socket");
	
	ec = setsockopt(socketfd, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));
	if(ec == -1) err_quit("setsockopt");
	
	ec = setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));
	if(ec == -1) err_quit("broadcast");
	
	ec = bind(socketfd, (struct sockaddr*) &sin, sizeof(sin));
	if(ec == -1) err_quit("bind");
	
	int loop = 0;
	//for(int r = 0; r < 8 * 1000 * 32; r++){
		//cout << r << "---------------------------------------------\n";
	while(1){
		loop++;
		memset(buffer, 0, MAXSIZE);
		int len = recvfrom(socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&csin, &csinlen);
		//cout << len << " " <<  loop << "---------------------------------------------\n";
		//cout << len << '\n';
		if(buffer[21] == 'C'){
			cout << "***************\n";
			cout << len << '\n';
			break;
		}
		//
		char checksum = 0;
		for(int i = 20; i < len; i++) checksum ^= buffer[i];
		if(checksum != 0) continue;
		//
		ipheader(ip, 6);
		buffer[20] = 0;
		//cout << "server: " << buffer[21] << buffer[22]  << buffer[23] << buffer[24] << buffer[25] << '\n';
		for(int i = 21; i < 26; i++){
			buffer[20] ^= buffer[i];
		}
		
		//for(int i = 0; i < 3; i++) sendto(socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sin, sizeof(sin));
		packetBuf.push_back(string(buffer + 21));
	}
	
	for(string in : packetBuf){
		string file = in.substr(0, 3);
		int index;
		sscanf(in.substr(3, 2).c_str(), "%d", &index);
		dataBuf[file][index] = in.substr(5);
	}
	
	for(int i = 0; i < 1000; i++){
		if(i < 10) 			initFile = "00" + to_string(i);
		else if(i < 100) 	initFile = "0" + to_string(i);
		else				initFile = to_string(i);
		ofstream out(path+"/000"+initFile, ios_base::out);
		int j = 0;
		string tmp = "";
		while(dataBuf[initFile][j] != ""){
			tmp += dataBuf[initFile][j];
			j++;
		}
		out << tmp;
		out.close();
	}
	cout << "*****************************************************\n";
	memset(buffer, 0, sizeof(buffer));
	ipheader(ip, 2);
	buffer[20] = 'S';
	buffer[21] = 'S';
	while(1) sendto(socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sin, sizeof(sin));
	close(socketfd);
	return;
}

int main(int argc, char*argv[]){
	if(argc < 4) err_quit("argc");
	path = string(argv[1]);
	sscanf(argv[2], "%d", &numFiles);
	broadcastAddress = string(argv[3]);
	setvbuf(stdout, NULL, _IONBF, 0);
	server();
	return 0;
}