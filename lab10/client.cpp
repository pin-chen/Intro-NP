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

#define MAXLINE 50000
#define PACKETSIZE 1100
#define SEGSIZE 1000

int numFiles;
string path, broadcastAddress;

map<string, bool> isRecv;

struct perFile{
    char packet[21 + PACKETSIZE];
};

perFile F[1000][40];
int num_seg[1000];

void ipheader(struct iphdr *ip, size_t len){
	ip->version  = 4;
	ip->ihl      = 5;
	ip->tos      = 16;
	ip->tot_len  = sizeof(struct iphdr) + len;
	ip->id       = htons(11111);
	ip->ttl      = 64;
	ip->protocol = 161;
	ip->check = 0;
	ip->saddr = inet_addr("192.168.0.2");
	ip->daddr = inet_addr(broadcastAddress.c_str());
}

void server(){
	cout << "c_server\n";
	
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
		isRecv[initFile] = false;
	}
	
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(9996);
	sin.sin_addr.s_addr = inet_addr(broadcastAddress.c_str());
	
	socketfd = socket(AF_INET, SOCK_RAW, IPPROTO_XDP);
	if(socketfd == -1) err_quit("socket");
	
	ec = setsockopt(socketfd, IPPROTO_IP, IP_HDRINCL, &val, sizeof(val));
	if(ec == -1) err_quit("setsockopt");
	
	ec = setsockopt(socketfd, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val));
	if(ec == -1) err_quit("broadcast");
	
	ec = bind(socketfd, (struct sockaddr*) &sin, sizeof(sin));
	if(ec == -1) err_quit("bind");
	
	while(1){
		memset(buffer, 0, MAXSIZE);
		int len = recvfrom(socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&csin, &csinlen);
		if(buffer[20] == 'S' && buffer[21] == 'S') exit(1);
		char checksum = buffer[20];
		string xxx;
		for(int i = 21; i < 26; i++){
			checksum ^= buffer[i];
			xxx.push_back(buffer[i]);
		} 
		if(checksum != 0) continue;
		//cout << "ACK" << buffer[21] << buffer[22]  << buffer[23] << buffer[24] << buffer[25] << '\n';
		isRecv[xxx] = true;
	}
	return;
}

void client(){
	cout << "c_client\n";
	
	int socketfd, val = 1, ec;
	char buffer[MAXSIZE];
	struct iphdr *ip = (struct iphdr *) buffer;
	struct sockaddr_in sin;
	
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
	
	for (int f_seq=0; f_seq<1000; f_seq++){
        /* Construct Pakcet for initial and un-ack */
        char filename[200] = {}, data[MAXLINE] = {},
            header[11], 
            segments[50][SEGSIZE + 1] = {};
        int num_data, inputFD;

        // Construct Packet
        sprintf(filename, "%s/%06d", path.c_str(), f_seq);
        inputFD = open(filename, O_RDONLY);
        num_data = read(inputFD, data, MAXLINE);

        // slice to segments
        num_seg[f_seq] = (num_data % SEGSIZE == 0) ? num_data / SEGSIZE : num_data / SEGSIZE + 1;
        for (int i=0, i_data=0, j=0; i_data < num_data; i_data++){
            i = i_data / SEGSIZE;           // segments index
            j = i_data % SEGSIZE;           // segments offset
            if (i != num_seg[f_seq] - 1)           // for segments size calculation
                segments[i][1000] = '\0';
            segments[i][j] = data[i_data];
        }

        // construct header and send
        for (int i_seg=0; i_seg<num_seg[f_seq]; i_seg++){
            // construc pkt
            sprintf(header, "%03d%02d", f_seq, i_seg);
			
            sprintf(&F[f_seq][i_seg].packet[21], "%s%s", header, segments[i_seg]);
			int xlen = 0;
			F[f_seq][i_seg].packet[20] = 0;
			for(int x = 21; F[f_seq][i_seg].packet[x] != 0; x++){
				xlen++;
				F[f_seq][i_seg].packet[20] ^= F[f_seq][i_seg].packet[x];
			}
			struct iphdr *ip2 = (struct iphdr *) F[f_seq][i_seg].packet;
			ipheader(ip2, 6 + xlen);
        }
    }
	int loop = 0;
	for(int i = 0; i < 5; i++){
	//while(1){
		//loop++;
		//cout << loop << '\n';
		bool all_set = true;
        for (int f_seq=0; f_seq<1000; f_seq++){
			
            char serv_reply[20] = {};
            // send
            for (int i_seg=0; i_seg<num_seg[f_seq]; i_seg++){
                // Skip if received ACK
                sprintf(serv_reply, "%03d%02d", f_seq, i_seg);
                string map_idx(serv_reply);
                if (isRecv.count(map_idx) && isRecv[map_idx])
                    continue;
				//
				//cout << "loop: " << loop << " f_seq: " << f_seq << " i_seg: " << i_seg << '\n';
                // set map
                if (!isRecv.count(map_idx))
                    isRecv[map_idx] = false;
                // not all done
                all_set = false;
                // send packet
                sendto(socketfd, F[f_seq][i_seg].packet, sizeof(F[f_seq][i_seg].packet), 0, (struct sockaddr*)&sin, sizeof(sin));
				usleep(100);
			}
        }
        if (all_set) break;
	}
	
	while(1){
		memset(buffer, 0 , sizeof(buffer));
		buffer[20] = 'C';
		buffer[21] = 'C';
		ipheader(ip, 2);
		sendto(socketfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&sin, sizeof(sin));
		usleep(100);
	}
	return;
}

int main(int argc, char*argv[]){
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	if(argc < 4) err_quit("argc");
	path = string(argv[1]);
	sscanf(argv[2], "%d", &numFiles);
	broadcastAddress = string(argv[3]);
	std::thread t(client);
	server();
	t.join();
	return 0;
}