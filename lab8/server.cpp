#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#define SA struct sockaddr
#define err_quit(m) { perror(m); exit(-1); }
using namespace std;

const int n = 2147493647;
map<string, vector<string>> dataBuf;
vector<string> packetBuf;

int main(int argc, char *argv[]) {
	int s, port;
	string path, initFile;
	struct sockaddr_in sin, csin;
	socklen_t csinlen = sizeof(csin);
	char buf[1200];

	path = string(argv[1]);
	sscanf(argv[3], "%d", &port);
	
	for(int i = 0; i < 1000; i++){
		if(i < 10) 			initFile = "00" + to_string(i);
		else if(i < 100) 	initFile = "0" + to_string(i);
		else				initFile = to_string(i);
		dataBuf[initFile].resize(40);
	}

	setvbuf(stdout, NULL, _IONBF, 0);
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons(port);

	if((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		err_quit("socket");

	if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) < 0)
		err_quit("buffer");

	if(bind(s, (struct sockaddr*) &sin, sizeof(sin)) < 0)
		err_quit("bind");
	
	while(1) {
		memset(buf, '\0', sizeof(buf));
		int len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr*) &csin, &csinlen);
		if(len == 2) break;
		for(int i = 0; i < 11; i++) sendto(s, buf, 5, 0, (struct sockaddr*)&csin, csinlen);
		packetBuf.push_back(string(buf));
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

	char over[] = "AA";
	while(1) sendto(s, over, 2, 0, (struct sockaddr*)&csin, csinlen);
	close(s);
	exit(0);
}

		//timeval start, end;
		//double elapse;
		//gettimeofday(&start, NULL);

		//gettimeofday(&end, NULL);
		//elapse = (end.tv_sec - start.tv_sec) * 1000.0;
		//elapse += (end.tv_usec - start.tv_usec) / 1000.0;
		//printf("elapse: %f\n", elapse);