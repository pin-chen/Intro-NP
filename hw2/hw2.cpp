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

//#define DEBUG

using namespace std;

typedef struct soa_header{
	uint32_t serial;
	uint32_t refresh;
	uint32_t retry;
	uint32_t expire;
	uint32_t minimum;
}__attribute__((packed)) soa;

typedef struct mx_header{
	uint16_t preference;
}__attribute__((packed)) mx;

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
		}else if(src[cur] == '\r' || src[cur] == '\n'){
			continue;
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

int parser(struct packet&tmp ,unsigned char *buf){
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
	return index;
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
#ifdef DEBUG
		cout << "forward len: " << len << '\n';
#endif
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

string Reverse(string tmp){
	int len = tmp.size();
	for(int i = 0; i < len / 2; i++){
		char t = tmp[i];
		tmp[i] = tmp[len - i - 1];
		tmp[len - i - 1] = t;
	}
	return tmp;
}

string domain_check(struct packet&tmp){
	string qname = Reverse(tmp.questions[0].qname);
	for(auto d : dnsInfo){
		string dname = Reverse(d.first);
		int len = qname.size() < dname.size() ? qname.size() : dname.size();
		bool same = true;
		for(int i = 0; i < len; i++){
			if(qname[i] != dname[i]){
				same = false;
				break;
			}
		}
		if(same) return d.first;
	}
	return "";
}

void strEncode(unsigned char *buf, string tmp){
#ifdef DEBUG
	cout << "string: " << tmp << '\n';
	cout << "size: " << tmp.size() << '\n';
#endif
	int index = 0, i = 0, j = 0;
	while(index < tmp.size()){
		int num = 0;
		for(j = index; j < tmp.size() && tmp[j] != '.'; j++) num++;
		buf[i++] = num;
		while(num-- > 0){
			buf[i++] = tmp[index++];
		}
		index++;
	}
	buf[i++] = 0;
#ifdef DEBUG
	cout << "name: ";
	for(int k = 0; k < i; k++){
		if(buf[k] < 30) cout << '\n' << int(buf[k]) << '\n';
		else cout << buf[k];
	}
	cout << "\n";
#endif
}

void make_soa_rr(string name, per_data &soa_data, unsigned char *buf, int &index){
	strEncode((unsigned char *)&buf[index], name);
	index += name.size() + 1;
	answer *ANSWER = (answer*)&buf[index];
	ANSWER->atype = htons(SOA);
	ANSWER->aclass = htons(IN);
	ANSWER->ttl = htonl(stoi(soa_data.ttl.c_str()));
	index += sizeof(answer);
	int len = index;
	vector<string> soa_tmp;
	splitArgument(soa_tmp, soa_data.payload, ' ');
	strEncode((unsigned char *)&buf[index], soa_tmp[0]);
	index += soa_tmp[0].size() + 1;
	strEncode((unsigned char *)&buf[index], soa_tmp[1]);
	index += soa_tmp[1].size() + 1;
	soa *_SOA = (soa*)&buf[index];
	_SOA->serial = htonl(stoi(soa_tmp[2].c_str()));
	_SOA->refresh = htonl(stoi(soa_tmp[3].c_str()));
	_SOA->retry = htonl(stoi(soa_tmp[4].c_str()));
	_SOA->expire = htonl(stoi(soa_tmp[5].c_str()));
	_SOA->minimum = htonl(stoi(soa_tmp[6].c_str()));
	index += sizeof(soa);
	ANSWER->rdlength = htons(index - len);
}

void make_ns_rr(string name, per_data &ns_data, unsigned char *buf, int &index){
	strEncode((unsigned char *)&buf[index], name);
	index += name.size() + 1;
	answer *ANSWER = (answer*)&buf[index];
	ANSWER->atype = htons(NS);
	ANSWER->aclass = htons(IN);
	ANSWER->ttl = htonl(stoi(ns_data.ttl.c_str()));
	index += sizeof(answer);
	strEncode((unsigned char *)&buf[index], ns_data.payload);
	index += ns_data.payload.size() + 1;
	ANSWER->rdlength = htons(ns_data.payload.size() + 1);
}

void make_mx_rr(string name, per_data &mx_data, unsigned char *buf, int &index){
	strEncode((unsigned char *)&buf[index], name);
	index += name.size() + 1;
	answer *ANSWER = (answer*)&buf[index];
	ANSWER->atype = htons(MX);
	ANSWER->aclass = htons(IN);
	ANSWER->ttl = htonl(stoi(mx_data.ttl.c_str()));
	index += sizeof(answer);
	int len = index;
	vector<string> mx_tmp;
	splitArgument(mx_tmp, mx_data.payload, ' ');
	mx *_MX = (mx*)&buf[index];
	_MX->preference = htons(stoi(mx_tmp[0].c_str()));
	index += sizeof(mx);
	strEncode((unsigned char *)&buf[index], mx_tmp[1]);
	index += mx_tmp[1].size() + 1;
	ANSWER->rdlength = htons(index - len);
}

void make_txt_rr(string name, per_data &txt_data, unsigned char *buf, int &index){
	strEncode((unsigned char *)&buf[index], name);
	index += name.size() + 1;
	answer *ANSWER = (answer*)&buf[index];
	ANSWER->atype = htons(TXT);
	ANSWER->aclass = htons(IN);
	ANSWER->ttl = htonl(stoi(txt_data.ttl.c_str()));
	index += sizeof(answer);
	unsigned char *str = (unsigned char *)&buf[index];
	str[0] = txt_data.payload.size() - 2;
	for(int i = 1; i < txt_data.payload.size() - 1; i++){
		str[i] = txt_data.payload[i];
	}
	index += txt_data.payload.size() - 1;
	ANSWER->rdlength = htons(txt_data.payload.size() - 1);
}

void make_cname_rr(string name, per_data &cname_data, unsigned char *buf, int &index){
	strEncode((unsigned char *)&buf[index], name);
	index += name.size() + 1;
	answer *ANSWER = (answer*)&buf[index];
	ANSWER->atype = htons(CNAME);
	ANSWER->aclass = htons(IN);
	ANSWER->ttl = htonl(stoi(cname_data.ttl.c_str()));
	index += sizeof(answer);
	strEncode((unsigned char *)&buf[index], cname_data.payload);
	index += cname_data.payload.size() + 1;
	ANSWER->rdlength = htons(cname_data.payload.size() + 1);
}

void make_a_rr(string name, per_data &a_data, unsigned char *buf, int &index){
	strEncode((unsigned char *)&buf[index], name);
	index += name.size() + 1;
	answer *ANSWER = (answer*)&buf[index];
	ANSWER->atype = htons(A);
	ANSWER->aclass = htons(IN);
	ANSWER->ttl = htonl(stoi(a_data.ttl.c_str()));
	index += sizeof(answer);
	inet_pton(AF_INET, a_data.payload.c_str(), (in_addr *)&buf[index]);
	index += 4;
	ANSWER->rdlength = htons(4);
}

void make_aaaa_rr(string name, per_data &aaaa_data, unsigned char *buf, int &index){
	strEncode((unsigned char *)&buf[index], name);
	index += name.size() + 1;
	answer *ANSWER = (answer*)&buf[index];
	ANSWER->atype = htons(AAAA);
	ANSWER->aclass = htons(IN);
	ANSWER->ttl = htonl(stoi(aaaa_data.ttl.c_str()));
	index += sizeof(answer);
	inet_pton(AF_INET6, aaaa_data.payload.c_str(), (in_addr *)&buf[index]);
	index += 16;
	ANSWER->rdlength = htons(16);
}

int make_packet(string domain, struct packet&tmp, unsigned char *buf, int index){
	dns *DNS = (dns*)buf;
	DNS->qr = 1;
	DNS->aa = 1;
	DNS->ra = 1;
	DNS->qdcount = htons(1);
	uint16_t anc = 0, nsc = 0, arc = 0;
	switch(tmp.questions[0].qtype){
		case A:{
			vector<string> c_name;
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "CNAME"){
					string host = "";
					if(r.host[r.host.size() - 1] == '.'){
						host = r.host;
					}else{
						host = r.host + "." + domain;
					}
					if(host != tmp.questions[0].qname) continue;
					anc++;
					c_name.push_back(r.payload);
					make_cname_rr(tmp.questions[0].qname, r, buf, index);
				}
			}
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "A"){
					string host = "";
					if(r.host[r.host.size() - 1] == '.'){
						host = r.host;
					}else{
						host = r.host + "." + domain;
					}
					if(host != tmp.questions[0].qname) continue;
					anc++;
					make_a_rr(host, r, buf, index);
				}
			}
			for(auto qname : c_name){
				for(auto r : dnsInfo[domain].records){
					if(r.rtype == "A"){
						string host = "";
						if(r.host[r.host.size() - 1] == '.'){
							host = r.host;
						}else{
							host = r.host + "." + domain;
						}
						if(host != qname) continue;
						anc++;
						make_a_rr(host, r, buf, index);
					}
				}
			}
			if(anc == 0) break;
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "NS"){
					nsc++;
					make_ns_rr(domain, r, buf, index);
				}
			}
			break;
		}
		case NS:{
			vector<string> ns_name;
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "NS"){
					anc++;
					ns_name.push_back(r.payload);
					make_ns_rr(domain, r, buf, index);
				}
			}
			for(auto qname : ns_name){
				for(auto r : dnsInfo[domain].records){
					if(r.rtype == "A"){
						string host = "";
						if(r.host[r.host.size() - 1] == '.'){
							host = r.host;
						}else{
							host = r.host + "." + domain;
						}
						if(host != qname) continue;
						arc++;
						make_a_rr(host, r, buf, index);
					}
				}
			}
			break;
		}
		case MX:{
			vector<string> mx_name;
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "MX"){
					anc++;
					vector<string> temp;
					splitArgument(temp, r.payload, ' ');
					mx_name.push_back(temp[1]);
					make_mx_rr(domain, r, buf, index);
				}
			}
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "NS"){
					nsc++;
					make_ns_rr(domain, r, buf, index);
				}
			}
			for(auto qname : mx_name){
				for(auto r : dnsInfo[domain].records){
					if(r.rtype == "A"){
						string host = "";
						if(r.host[r.host.size() - 1] == '.'){
							host = r.host;
						}else{
							host = r.host + "." + domain;
						}
						if(host != qname) continue;
						arc++;
						make_a_rr(host, r, buf, index);
					}
				}
			}
			break;
		}
		case SOA:{
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "SOA"){
					anc++;
					make_soa_rr(domain, r, buf, index);
					break;
				}
			}
			break;
		}
		case CNAME:{
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "CNAME"){
					string host = "";
					if(r.host[r.host.size() - 1] == '.'){
						host = r.host;
					}else{
						host = r.host + "." + domain;
					}
					if(host != tmp.questions[0].qname) continue;
					anc++;
					make_cname_rr(tmp.questions[0].qname, r, buf, index);
				}
			}
			break;
		}
		case TXT:{
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "TXT"){
					anc++;
					make_txt_rr(domain, r, buf, index);
				}
			}
			break;
		}
		case AAAA:{
			vector<string> c_name;
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "CNAME"){
					string host = "";
					if(r.host[r.host.size() - 1] == '.'){
						host = r.host;
					}else{
						host = r.host + "." + domain;
					}
					if(host != tmp.questions[0].qname) continue;
					anc++;
					c_name.push_back(r.payload);
					make_cname_rr(tmp.questions[0].qname, r, buf, index);
				}
			}
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "AAAA"){
					string host = "";
					if(r.host[r.host.size() - 1] == '.'){
						host = r.host;
					}else{
						host = r.host + "." + domain;
					}
					if(host != tmp.questions[0].qname) continue;
					anc++;
					make_aaaa_rr(host, r, buf, index);
				}
			}
			for(auto qname : c_name){
				for(auto r : dnsInfo[domain].records){
					if(r.rtype == "AAAA"){
						string host = "";
						if(r.host[r.host.size() - 1] == '.'){
							host = r.host;
						}else{
							host = r.host + "." + domain;
						}
						if(host != qname) continue;
						anc++;
						make_aaaa_rr(host, r, buf, index);
					}
				}
			}
			if(anc == 0) break;
			for(auto r : dnsInfo[domain].records){
				if(r.rtype == "NS"){
					nsc++;
					make_ns_rr(domain, r, buf, index);
				}
			}
			break;
		}
	}
	if(anc == 0){
		for(auto r : dnsInfo[domain].records){
			if(r.rtype == "SOA"){
				nsc++;
				make_soa_rr(domain, r, buf, index);
				break;
			}
		}
	}
	DNS->ancount = htons(anc);
	DNS->nscount = htons(nsc);
	DNS->arcount = htons(arc);
	return index;
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
		int index = parser(request, buf);
		//check own domain
		string domain = domain_check(request);
		
		if(domain != ""){//send resonse
			regex reg("^([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})\\.([0-9a-zA-Z]{1,61}\\.)*");
			if(regex_match(request.questions[0].qname, reg)){
				dns *DNS = (dns*)buf;
				DNS->qr = 1;
				DNS->aa = 1;
				DNS->ra = 1;
				DNS->qdcount = htons(1);
				DNS->ancount = htons(1);
				DNS->nscount = 0;
				DNS->arcount = 0;
				strEncode((unsigned char *)&buf[index], request.questions[0].qname);
				index += request.questions[0].qname.size() + 1;
				answer *ANSWER = (answer*)&buf[index];
				ANSWER->atype = htons(A);
				ANSWER->aclass = htons(IN);
				ANSWER->ttl = htonl(1);
				index += sizeof(answer);
				int iip;
				int cip = 0;
				for(iip = 0; iip < request.questions[0].qname.size(); iip++){
					if(request.questions[0].qname[iip] == '.') cip++;
					if(cip > 3) break;
				}
				inet_pton(AF_INET, request.questions[0].qname.substr(0, iip).c_str(), (in_addr *)&buf[index]);
				index += 4;
				ANSWER->rdlength = htons(4);
			}else{
				index = make_packet(domain, request, buf, index);
			}
#ifdef DEBUG
			cout << "check domain: " << domain << '\n';
#endif
			sendto(sockfd, buf, index, 0, (struct sockaddr*)&csin, sizeof(csin));
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