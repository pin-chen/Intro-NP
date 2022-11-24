/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */
#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

#include "base64.c"

char * sha1sum(const char *s, int slen, char *output) {
	int i;
	unsigned char cksum[20];
	if(SHA1((const unsigned char*) s, slen, cksum) == NULL)
		return NULL;
	for(i = 0; i < 20; i++) snprintf(output+i*2, 4, "%2.2x", cksum[i]);
	return output;
}

int main(int argc, char *argv[]) {
	int len, zeros, i, z;
	char input[128], cksum[64];

	if(argc < 3) return -fprintf(stderr, "usage: %s prefix leading-zeros\n", argv[0]);

	len = snprintf(input, sizeof(input), "%s", argv[1]);
	zeros = strtol(argv[2], NULL, 0);
	printf("prefix %s len %d\n", input, zeros);

	for(i = 0; i < 1073741824; i++) {
		*((int*) &input[len]) = i;
		sha1sum(input, len+sizeof(i), cksum);
		for(z = 0; z < zeros; z++)
			if(cksum[z] != '0') break;
		if(z == zeros) {
			printf("Solved: %d\n", i);
			printf("%s\n", b64encode((unsigned char*) &i, sizeof(i), (unsigned char *) input, &z));
			printf("%s\n", cksum);
			break;
		}
	}

	return 0;
}

