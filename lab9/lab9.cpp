#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h> // bzero()
#include <sys/socket.h>
#include <unistd.h> // read(), write(), close()
#include <arpa/inet.h> // inet_addr()
#include <time.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <iostream>
#include <sys/un.h>
#include <vector>
#define pii pair<int,int>
#define f first
#define s second
#define SA struct sockaddr
#define SERVER_PATH "/sudoku.sock"
using namespace std;
int square[9][9];
vector<pii> empty;

bool checker(int tmp, int index){
	for(int i = 0; i < 9; i++){
		if(tmp == square[empty[index].f][i]) return false;
		if(tmp == square[i][empty[index].s]) return false;
	}
	int x = empty[index].f / 3;
	int y = empty[index].s / 3;
	for(int j = x * 3; j < (x + 1) * 3; j++){
		for(int k = y * 3; k < (y + 1) * 3; k++){
			if(tmp == square[j][k]) return false;
		}
	}
	return true;
}

bool solever(int index){
	if(index >= empty.size()){
		return true;
	} 
	for(int i = 1; i < 10; i++){
		if(checker(i, index) == true){
			square[empty[index].f][empty[index].s] = i;
			if(solever(index + 1) == true) return true;
		}
		square[empty[index].f][empty[index].s] = -1;
	}
	return false;
}

int main(int argc, char**argv){
	int sockfd;
    struct sockaddr_un servaddr;
	sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("Socket Error\n");
        exit(1);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, SERVER_PATH);
	int e = connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
    if (e == -1) {
        perror("Connect Error\n");
        exit(1);
    }
	dup2(sockfd, 0);
	dup2(sockfd, 1);
	string s, trash;
	cout << "S\n" << flush;
	while(getline(cin, s)) if(s[0] == 'O') break;
	for(int i = 4; i < s.size(); i++){
		int index_i = (i - 4) / 9;
		int index_j = (i - 4) % 9;
		if(s[i] == '.'){
			empty.push_back({index_i, index_j});
			square[index_i][index_j] = -1;
		}else{
			square[index_i][index_j] = s[i] - '0';
		}
	}
	solever(0);
	for(auto ans : empty){
		cout << "V " << ans.f << " " << ans.s << " " << square[ans.f][ans.s] << '\n' << flush;
		getline(cin, trash);
	}
	cout << "C\n";
	getline(cin, s);
	cerr << s << '\n';
	close(sockfd);
	return 0;
}
