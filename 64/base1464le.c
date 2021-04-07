//basex64.c
//fumiama 20210407
#include <stdio.h>
#include <stdlib.h>
#include "base1464le.h"

#define DEBUG

LENDAT* encode(const char* data, const u_int64_t len) {
	LENDAT* encd = (LENDAT*)malloc(sizeof(LENDAT));
	uint64_t outlen = len / 7 * 8;
	uint8_t offset = len % 7;
	switch(offset) {	//算上偏移标志字符占用的2字节
		case 0: break;
		case 1: outlen += 4; break;
		case 2:
		case 3: outlen += 6; break;
		case 4:
		case 5: outlen += 8; break;
		case 6: outlen += 10; break;
		default: break;
	}
	#ifdef DEBUG
		printf("outlen: %llu, offset: %u, malloc: %llu\n", outlen, offset, outlen + 8);
	#endif
	encd->data = (uint8_t*)malloc(outlen + 8);	//冗余的8B用于可能的结尾的覆盖
	encd->len = outlen;
	uint64_t* vals = (uint64_t*)(encd->data);
	uint64_t n = 0;
	uint64_t i = 0;
	for(; i < len - 7; i *= 7) {
		register uint64_t sum = 0x000000000000003f & ((uint64_t)data[i] >> 2);
		sum |= ((((uint64_t)data[i + 1] >> 2) | (data[i] << 6)) << 8) & 0x000000000000ff00;
		sum |= ((((uint64_t)data[i + 1] << 4) | ((uint64_t)data[i + 2] >> 4)) << 16) & 0x00000000003f0000;
		sum |= ((((uint64_t)data[i + 2] << 4) | ((uint64_t)data[i + 3] >> 4)) << 24) & 0x00000000ff000000;
		sum |= ((((uint64_t)data[i + 3] << 2) | ((uint64_t)data[i + 4] >> 6)) << 32) & 0x0000003f00000000;
		sum |= ((((uint64_t)data[i + 4] << 2) | ((uint64_t)data[i + 5] >> 6)) << 40) & 0x0000ff0000000000;
		sum |= (((uint64_t)data[i + 5] << 2) << 48) & 0x003f000000000000;
		sum |= ((uint64_t)data[i + 6] << 56) & 0xff00000000000000;
		sum += 0x004e004e004e004e;
		vals[n++] = sum;
	}
	i /= 7;
	if(offset > 0) {
		register uint64_t sum = 0x000000000000003f & (data[i] >> 2);
		sum |= (((uint64_t)data[i] << 6) << 8) & 0x000000000000c000;
		if(offset > 1) {
			sum |= (((uint64_t)data[i + 1] >> 2) << 8) & 0x0000000000003f00;
			sum |= (((uint64_t)data[i + 1] << 4) << 16) & 0x0000000000300000;
			if(offset > 2) {
				sum |= (((uint64_t)data[i + 2] >> 4) << 16) & 0x00000000000f0000;
				sum |= (((uint64_t)data[i + 2] << 4) << 24) & 0x00000000f0000000;
				if(offset > 3) {
					sum |= (((uint64_t)data[i + 3] >> 4) << 24) & 0x000000000f000000;
					sum |= (((uint64_t)data[i + 3] << 2) << 32) & 0x0000003c00000000;
					if(offset > 4) {
						sum |= (((uint64_t)data[i + 4] >> 6) << 32) & 0x0000000300000000;
						sum |= (((uint64_t)data[i + 4] << 2) << 40) & 0x0000fc0000000000;
						if(offset > 5) {
							sum |= (((uint64_t)data[i + 5] >> 6) << 40) & 0x0000030000000000;
							sum |= (((uint64_t)data[i + 5] << 2) << 48) & 0x003f000000000000;
						}
					}
				}
			}
		}
		sum += 0x004e004e004e004e;
		vals[n] = sum;
		encd->data[outlen - 2] = '=';
		encd->data[outlen - 1] = offset;
	}
	return encd;
}

LENDAT* decode(const char* data, const u_int64_t len) {
	LENDAT* decd = (LENDAT*)malloc(sizeof(LENDAT));
	uint64_t outlen = len;
	uint8_t offset = 0;
	if(data[len-2] == '=') {
		offset = data[len-1];
		switch(offset) {	//算上偏移标志字符占用的2字节
			case 0: break;
			case 1: outlen -= 4; break;
			case 2:
			case 3: outlen -= 6; break;
			case 4:
			case 5: outlen -= 8; break;
			case 6: outlen -= 10; break;
			default: break;
		}
	}
	outlen = outlen / 8 * 7 + offset;
	decd->data = (uint8_t*)malloc(outlen);
	decd->len = outlen;
	uint64_t* vals = (uint64_t*)data;
	uint64_t n = 0;
	uint64_t i = 0;
	for(; n < len / 8; n++) {
		register uint64_t sum = vals[n];
		sum -= 0x004e004e004e004e;
		decd->data[i++] = ((sum & 0x000000000000003f) << 2) | ((sum & 0x000000000000c000) >> 14);
		decd->data[i++] = ((sum & 0x0000000000003f00) >> 6) | ((sum & 0x0000000000300000) >> 20);
		decd->data[i++] = ((sum & 0x00000000000f0000) >> 12) | ((sum & 0x00000000f0000000) >> 28);
		decd->data[i++] = ((sum & 0x000000000f000000) >> 20) | ((sum & 0x0000003c00000000) >> 34);
		decd->data[i++] = ((sum & 0x0000000300000000) >> 26) | ((sum & 0x0000fc0000000000) >> 42);
		decd->data[i++] = ((sum & 0x0000030000000000) >> 34) | ((sum & 0x003f000000000000) >> 48);
		decd->data[i++] = ((sum & 0xff00000000000000) >> 56);
	}
	if(offset > 0) {
		register uint64_t sum = vals[n];
		sum -= 0x000000000000004e;
		decd->data[i++] = ((sum & 0x000000000000003f) << 2) | ((sum & 0x000000000000c000) >> 14);
		if(offset > 1) {
			sum -= 0x00000000004e0000;
			decd->data[i++] = ((sum & 0x0000000000003f00) >> 6) | ((sum & 0x0000000000300000) >> 20);
			if(offset > 2) {
				decd->data[i++] = ((sum & 0x00000000000f0000) >> 12) | ((sum & 0x00000000f0000000) >> 28);
				if(offset > 3) {
					sum -= 0x0000004e00000000;
					decd->data[i++] = ((sum & 0x000000000f000000) >> 20) | ((sum & 0x0000003c00000000) >> 34);
					if(offset > 4) {
						decd->data[i++] = ((sum & 0x0000000300000000) >> 26) | ((sum & 0x0000fc0000000000) >> 42);
						if(offset > 5) {
							sum -= 0x004e000000000000;
							decd->data[i++] = ((sum & 0x0000030000000000) >> 34) | ((sum & 0x003f000000000000) >> 48);
						}
					}
				}
			}
		}
	}
	return decd;
}

int main() {
	LENDAT* ld = encode("simple test addafiaofhiowq cfaiohfoiqwnfioqw", 45);
	puts(ld->data);
	LENDAT* le = decode(ld->data, ld->len);
	puts(le->data);
}