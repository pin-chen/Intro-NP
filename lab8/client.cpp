#include <iostream>
#include <string>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define MAXLINE 50000
#define PACKETSIZE 1100
#define SEGSIZE 1000
#define NUMFILESLEEP 8
#define USLEEPTIME 90
using namespace std;

map<string, bool> datamap;

struct perFile{
    char packet[PACKETSIZE];
};

perFile F[1000][40];
int num_seg[1000];

int main(int argc, char **argv){
    int					listenFD, connFD, num_total_file, size;
	int32_t             servport;
	socklen_t			clilen;
	struct sockaddr_in	servaddr;
    char path_to_file[100] = {}, servip[100] = {};

	setvbuf(stdin, NULL, _IONBF, 0);

    // Read from arguments
    if (argc >= 5){
        strcpy(path_to_file, argv[1]);
        num_total_file = atoi(argv[2]);
        servport = atoi(argv[3]);
        strcpy(servip, argv[4]);
    }

    // Build connection
    connFD = socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(servport);
    size = sizeof(servaddr);
    // Unblock
    int flags = fcntl(connFD, F_GETFL, 0);
    fcntl(connFD, F_SETFL, flags | O_NONBLOCK);

    for (int f_seq=0; f_seq<1000; f_seq++){
        /* Construct Pakcet for initial and un-ack */
        char filename[200] = {}, data[MAXLINE] = {},
            header[11], 
            segments[50][SEGSIZE + 1] = {};
        int num_data, inputFD;

        // Construct Packet
        sprintf(filename, "%s/%06d", path_to_file, f_seq);
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
            sprintf(F[f_seq][i_seg].packet, "%s%s", header, segments[i_seg]);
        }
    }

    // Read file and send
    while (1){
        bool all_set = true;
        for (int f_seq=0; f_seq<1000; f_seq++){
            char serv_reply[20] = {};
            // send
            for (int i_seg=0; i_seg<num_seg[f_seq]; i_seg++){
                // Skip if received ACK
                sprintf(serv_reply, "%03d%02d", f_seq, i_seg);
                string map_idx(serv_reply);
                if (datamap.count(map_idx) && datamap[map_idx])
                    continue;
                // set map
                if (!datamap.count(map_idx))
                    datamap[map_idx] = false;
                // not all done
                all_set = false;
                // send packet
                sendto(connFD, F[f_seq][i_seg].packet, sizeof(F[f_seq][i_seg].packet), 0, (struct sockaddr*)&servaddr, size);
                // Ack
                memset(serv_reply, '\0', sizeof(serv_reply));
                while(recvfrom(connFD, serv_reply, 5, 0, (struct sockaddr*)&servaddr, &clilen) == 5){
                    serv_reply[6] = '\0';
                    string servac(serv_reply);
                    datamap[servac] = true;
                }
                usleep(USLEEPTIME);      // avoid overwhelming server buffer
            }
        }
        if (all_set) break;
    }
    char over[] = "AA";
    char X[2];
    while(1){
        sendto(connFD, over, 2, 0, (struct sockaddr*)&servaddr, size);
        recvfrom(connFD, X, 2, 0, (struct sockaddr*)&servaddr, &clilen);
        if(X[0] == 'A') exit(0);
    }
    return 0;
}