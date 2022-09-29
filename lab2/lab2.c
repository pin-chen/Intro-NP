#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 	//read write lseek
#include <stdint.h> 	//uint8_t uint16_t uint32_t
#include <arpa/inet.h> 	//htons honl ntohs ntohl
#include <fcntl.h> 	//open
#define G 1000000000
#define char unsigned char
#define htonll(x) (((uint64_t)htonl((x) & 0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) (((uint64_t)ntohl((x) & 0xFFFFFFFF) << 32) | ntohl((x) >> 32))

char contents[G];

typedef struct header_s {
	uint8_t P;
	uint8_t A;
	uint8_t K;
	uint8_t O;
	uint32_t of_str;
	uint32_t of_cont;
	uint32_t num_pack;
} __attribute__((packed)) header_t;

typedef struct FILE_E {
	uint32_t of_name;
	uint32_t size;
	uint32_t of_cont;
	uint64_t checksum;
} __attribute__((packed)) file_e;

int main(int argc, char *argv[]) {
	int i, j, k;
	if(argc != 3){
		perror("Wrong arguments!");
                exit(EXIT_FAILURE);
	}
	const char* dir = argv[2];
	const char* srcfile = argv[1];

	int srcfd = open(srcfile, O_RDONLY);
	if(srcfd == -1){
		perror("Open fail!");
		exit(EXIT_FAILURE);
	}
	
	header_t h;
	read(srcfd, &h, sizeof(h));
	if(h.P == 'P' && h.A == 'A' && h.K == 'K' && h.O == 'O'){
		puts("This is a PAKO file!");
	}else{
		puts("NOT PAKO file!");
		exit(EXIT_FAILURE);
	}
	printf("The number of files: %d\n", h.num_pack);
	
	file_e files[h.num_pack];
	for(i = 0; i < h.num_pack; i++){
		read(srcfd, &(files[i]), sizeof(files[i]));
		files[i].size = htonl(files[i].size);
		files[i].checksum = htonll(files[i].checksum);
	}

	lseek(srcfd, h.of_str, SEEK_SET);
	uint32_t str_len = h.of_cont - h.of_str;
	char string[str_len];
	memset(string, '\0', str_len);
	read(srcfd, string, str_len);

	lseek(srcfd, h.of_cont, SEEK_SET);
	//char contents[G];
	memset(contents, '\0', G);
	read(srcfd, contents, G);

	for(i = 0; i < h.num_pack; i++){
		uint32_t name_len;
		for(name_len = 0; string[files[i].of_name + name_len] != '\0'; name_len++);
		char name[name_len + 1];
		memset(name, '\0', name_len + 1);
		for(j = 0; j < name_len; j++){
			name[j] = string[files[i].of_name + j];
		}
		puts("--------------------------");
		printf("File name: %s\n", name);
		printf("File size: %d bytes\n", files[i].size);

		char content[files[i].size];
		memset(content, '\0', files[i].size);
		for(j = 0; j < files[i].size; j++){
			content[j] = contents[files[i].of_cont + j];
		}
		
		uint64_t check = 0;
		uint32_t end = 0;
		for(j = 0; j < files[i].size / 8 + 1; j++){
			uint64_t segment = 0;
			for(k = 0; k < 8 && end < files[i].size ; k++, end++){
				segment |= (uint64_t) content[end] << (k * 8);
			}
			check ^= segment;
		}

		if(check != files[i].checksum){
			puts("Bad checksum!");
			continue;
		}

		int len = strlen(dir) + strlen(name) + 2;
		char path[len];
		memset(path, '\0', len);
		const char* slash = "/";
		strcat(path, dir);
		if(dir[strlen(dir) - 1] != '/') 
			strcat(path, slash);
		strcat(path, name);
		int dstfd = open(path, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO );
		if(dstfd == -1){
			perror("Create Fail!");
			exit(EXIT_FAILURE);
		}
		write(dstfd, content, sizeof(content));

	}

	exit(EXIT_SUCCESS);
}
