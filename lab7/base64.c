/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */

static unsigned char *
b64encode(unsigned char *in, int inlen, unsigned char *output, int *outlen) {
	static char t[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int i, pad = 0;
	unsigned char ch = 0, *optr = output;
	if(output == 0) return output;
	if(inlen <= 0) { *output = 0; return output; }
	for(i = 0; i < inlen; i++) {
		switch(i % 3) {
		case 0:
			*optr++ = t[in[i]>>2];
			ch = (in[i] & 0x3)<<4;
			pad = 2;
			break;
		case 1:
			*optr++ = t[ch | (in[i]>>4)];
			ch = (in[i] & 0xf)<<2;
			pad = 1;
			break;
		case 2:
			*optr++ = t[ch | (in[i]>>6)];
			*optr++ = t[in[i] & 0x3f];
			pad = 0;
			break;
		}
	}
	if(pad > 0) *optr++ = t[ch];
	while(pad-- > 0) *optr++ = '=';
	*optr = '\0';
	*outlen = optr - output;
	return output;
}

static inline int
b64decode_code(int ch) {
	if(ch >= 'a' && ch <= 'z')	return 26 + ch - 'a';
	if(ch >= 'A' && ch <= 'Z')	return ch - 'A';
	if(ch >= '0' && ch <= '9')	return 52 + ch - '0';
	if(ch == '+') return 62;
	if(ch == '/') return 63;
	return -1;
}

static unsigned char *
b64decode(unsigned char *in, int inlen, unsigned char *output, int *outlen) {
	int i, code, coff = 0, flush = 1;
	unsigned char ch = 0, *optr = output;
	if(output == 0) return output;
	if(inlen <= 0) { *output = 0; return output; }
	for(i = 0; i < inlen; i++) {
		if(in[i] == '=') { break; }
		if((code = b64decode_code(in[i])) < 0) continue;
		switch(coff & 0x3) {
		case 0:
			ch = code<<2;
			flush = 1;
			break;
		case 1:
			*optr++ = ch | (code>>4);
			ch = (code & 0xf)<<4;
			flush = 0;
			break;
		case 2:
			*optr++ = ch | (code>>2);
			ch = (code & 0x3)<<6;
			flush = 0;
			break;
		case 3:
			*optr++ = ch | code;
			flush = ch = 0;
			break;
		}
		coff++;
	}
	if(flush) *optr++ = ch;
	*optr = '\0';
	*outlen = optr - output;
	return output;
}

#ifdef _MAIN_
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	int ilen, olen;
	unsigned char in[6*256], out[10*256];
	unsigned char * (*run)(unsigned char *, int, unsigned char *, int *) = b64encode;

	if(argc > 1) run = b64decode;

	while((ilen = read(0, in, sizeof(in))) > 0) {
		fprintf(stderr, "read: %d bytes\n", ilen);
		if(run(in, ilen, out, &olen) != NULL)
			write(1, out, olen);
	}
	write(1, "\n", 1);

	return 0;
}
#endif
