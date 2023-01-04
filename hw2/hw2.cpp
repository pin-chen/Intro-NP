#include "hw2.h"

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

#include <bits/stdc++.h>

#define	MAXLINE	40960
#define ERRORCODE -1
#define err_quit(m) { perror(m); exit(-1); }

#define DEBUG

using namespace std;

struct soa{
	int32_t ttl;
	uint16_t rclass;
	uint16_t rtype;
};

struct ns{
	string tmp;
};

struct mx{
	string tmp;
};

struct a{
	string tmp;
};

struct cname{
	string tmp;
};

struct aaaa{
	string tmp;
};

struct per_data{
	string host;
	string ttl;
	string rclass;
	string rtype;
	string payload;
};

struct domain_data{
	string path;
	vector<per_data> records;
};

string dns_ip;
map<string, domain_data> dnsInfo;

typedef struct dns_header{
	uint16_t id;
	
	uint8_t rd:1;
	uint8_t tc:1;
	uint8_t aa:1;
	uint8_t op:4;
	uint8_t qr:1;
	
	uint8_t rcode:4;
	uint8_t z:3;
	uint8_t ra:1;
	
	uint16_t qdcount;
	uint16_t ancount;
	uint16_t nscount;
	uint16_t arcount;
}__attribute__((packed)) dns;

typedef struct question_header{
	uint16_t qtype;
	uint16_t qclass;
}__attribute__((packed)) question;

typedef struct answer_header{
	uint16_t atype;
	uint16_t aclass;
	uint32_t ttl;
	uint16_t rdlength;
}__attribute__((packed)) answer;

struct query{
	string qname;
	uint16_t qtype;
	uint16_t qclass;
};

struct rr{
	string name;
	uint16_t atype;
	uint16_t aclass;
	uint32_t ttl;
	uint16_t rdlength;
	string rdata;
};

struct packet{
	dns header;
	vector<query> questions;
	vector<rr> answers;
	vector<rr> authority;
	vector<rr> additional;
};

void printHeader(dns & header){
	cout << "id: " << header.id << '\n';
	cout << "qr: " << header.qr << '\n';
	cout << "op: " << header.op << '\n';
	cout << "aa: " << header.aa << '\n';
	cout << "tc: " << header.tc << '\n';
	cout << "rd: " << header.rd << '\n';
	cout << "ra: " << header.ra << '\n';
	cout << "z: " << header.z << '\n';
	cout << "rcode: " << header.rcode << '\n';
	cout << "qdcount: " << header.qdcount << '\n';
	cout << "ancount: " << header.ancount << '\n';
	cout << "nscount: " << header.nscount << '\n';
	cout << "arcount: " << header.arcount << '\n';
}

void print_domain(){
	cout << "dns ip: " << dns_ip << '\n';
	for(auto d : dnsInfo){
		cout << "domain: " << d.first << '\n';
		cout << "path: " << d.second.path << '\n';
		for(auto r : d.second.records){
			cout << "-----------\n";
			cout << "host: " << r.host << '\n';
			cout << "ttl: "  << r.ttl << '\n';
			cout << "class: "  << r.rclass << '\n';
			cout << "type: "  << r.rtype << '\n';
			cout << "payload: "  << r.payload << '\n';
		}
		cout << "*********\n";
	}
}

void splitArgument(vector<string> &dst, string &src, char ccc){
	dst.clear();
	string arg;
	unsigned int cur = 0, charType = 0;
	for(; cur < src.size(); cur++){
		if(src[cur] == ccc){
			if(charType == 1){
				dst.push_back(arg);
				arg.clear();
				charType = 0;
			}
		}else{
			if(charType == 0) charType = 1;
			arg.push_back(src[cur]);
		}
	}
	if(arg.size() != 0){
		dst.push_back(arg);
	}
}

void server(int&sockfd, int port){
	int e;
	struct sockaddr_in sin;
	
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd == ERRORCODE) err_quit("socket");
	
	e = bind(sockfd, (struct sockaddr*)&sin, sizeof(sin));
	if(e == ERRORCODE) err_quit("bind");
}

void client(int&sockfd, int port){
	int e;
	struct sockaddr_in sin;
	
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd == ERRORCODE) err_quit("socket");
}

void header_parser(struct packet &tmp, dns *DNS){
	tmp.header.id = ntohs(DNS->id);
	tmp.header.rd = DNS->rd;
	tmp.header.tc = DNS->tc;
	tmp.header.aa = DNS->aa;
	tmp.header.op = DNS->op;
	tmp.header.qr = DNS->qr;
	tmp.header.rcode = DNS->rcode;
	tmp.header.z = DNS->z;
	tmp.header.ra = DNS->ra;
	tmp.header.qdcount = ntohs(DNS->qdcount);
	tmp.header.ancount = ntohs(DNS->ancount);
	tmp.header.nscount = ntohs(DNS->nscount);
	tmp.header.arcount = ntohs(DNS->arcount);
}

void name_parser(string &tmp, unsigned char *str){
	tmp.clear();
	int index = 0;
	uint8_t num = (uint8_t) str[index++];
	while(num != 0){
		for(uint8_t i = 0; i < num; i++){
			tmp.push_back(str[index++]);
		}
		tmp.push_back('.');
		num = (uint8_t) str[index++];
	}
}

void question_parser(struct query &tmp, question *q){
	tmp.qtype = ntohs(q->qtype);
	tmp.qclass = ntohs(q->qclass);
}

void parser(struct packet&tmp ,unsigned char *buf){
	int index = 0;
	header_parser(tmp, (dns*)buf);
	index += sizeof(dns);
#ifdef DEBUG
	printHeader(tmp.header);
#endif
	tmp.questions.resize(tmp.header.qdcount);
	for(uint16_t i = 0; i < tmp.header.qdcount; i++){
		name_parser(tmp.questions[i].qname, (unsigned char *)&buf[index]);
		index += tmp.questions[i].qname.size() + 1;
		question_parser(tmp.questions[i], (question*)&buf[index]);
		index += sizeof(question);
#ifdef DEBUG
		cout << "question index: " << i << '\n'; 
		cout << "qname: " << tmp.questions[i].qname << '\n';
		cout << "qtype: " << tmp.questions[i].qtype << '\n';
		cout << "qclass: " << tmp.questions[i].qclass << '\n';
#endif
	}
}

void forward(int &sockfd, struct sockaddr_in csin, unsigned char *buf, int len){
	int e;
	pid_t pid;
	pid = fork();
	if(pid < 0){
		err_quit("fork");
	}else if(pid == 0){
		int clifd;
		client(clifd, 11111);
		struct sockaddr_in sin;
		socklen_t sinlen;
		bzero(&sin, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = inet_addr(dns_ip.c_str());
		sin.sin_port = htons(53);
		e = sendto(clifd, buf, len, 0, (struct sockaddr*)&sin, sizeof(sin));
		if(e == ERRORCODE) err_quit("send to");
		len = recvfrom(clifd, buf, MAXLINE, 0, (struct sockaddr*) &sin, &sinlen);
		cout << "forward len: " << len << '\n';
		if(len == ERRORCODE) err_quit("recv");
		dns *DNS = (dns*) buf;
		DNS->aa = 0;
		DNS->ra = 1;
		e = sendto(sockfd, buf, len, 0, (struct sockaddr*)&csin, sizeof(csin));
		if(e == ERRORCODE) err_quit("send to");
		exit(0);
	}else{
		return;
	}
}

bool domain_check(struct packet&tmp){
	
}

void run(int port){
	int sockfd;
	unsigned char buf[MAXLINE];
	struct sockaddr_in csin;
	socklen_t csinlen = sizeof(csin);
	
	server(sockfd, port);
	
	while(1){
		//recv querry
		memset(buf, '\0', sizeof(buf));
		int len = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen);
#ifdef DEBUG
		cout << "len: " << len << '\n';
		for(int i = 0; i < len; i++) cout << buf[i];
		cout << '\n';
#endif
		//parse
		struct packet request;
		parser(request, buf);
		//check own domain
		if(domain_check(request)){//send resonse
			
		}else{//forward
			forward(sockfd, csin, buf, len);
		}
	}
}

void read_config(string path){
	ifstream in(path);
	getline(in, dns_ip);
	string tmp;
	while(getline(in, tmp)){
		int index;
		for(index = 0; tmp[index] != ','; index++);
		for(int i = index + 1; i < tmp.size(); i++){
			if(tmp[i] == '\r' || tmp[i] == '\n' || tmp[i] == ' ') break;
			dnsInfo[tmp.substr(0, index)].path.push_back(tmp[i]);
		}
	}
	in.close();
	tmp = "tmp";
	for(auto d : dnsInfo){
		ifstream fin(d.second.path);
		int i = 0;
		getline(fin, tmp);
		while(getline(fin, tmp)){
			vector<string> tdata;
			splitArgument(tdata, tmp, ',');
			per_data newR;
			dnsInfo[d.first].records.push_back(newR);
			dnsInfo[d.first].records[i].host = tdata[0];
			dnsInfo[d.first].records[i].ttl = tdata[1];
			dnsInfo[d.first].records[i].rclass = tdata[2];
			dnsInfo[d.first].records[i].rtype = tdata[3];
			dnsInfo[d.first].records[i].payload = tdata[4];
			i++;
		}
		fin.close();
	}
#ifdef DEBUG
	print_domain();
#endif
}

int main(int argc, char**argv){
	if(argc < 3){
		cerr << "argvc\n";
		exit(1);
	}
	int port;
	string path;
	
	port = stoi(argv[1]);
	path = string(argv[2]);
	
	setvbuf(stdin, NULL, _IONBF, 0);
	
	read_config(path);
	
	run(port);
	
	return 0;
}