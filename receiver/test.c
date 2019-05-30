#include <strings.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>

#include "receive.h"
#include "mainsnode.h"

unsigned char rev8(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void proc_datagram(size_t n, unsigned char * data) {
/*
transmitmanch((mF<<8)|mF); // Bias RX/TX / preamble. All "ones" so no matter which "one" it sees as a startbit, things will be fine once synchronized
transmitmanch((mF<<8)|mF); 
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mF<<8)|mF);
transmitmanch((mA<<8)|m5); // sync word, pre converted to machester 0xA5 = 0b10100101 -manch-> 0b1001 1001 0110 0110 = 0x9966
const uint32_t ID = 0x6A6A6A6A; //0x7777
transmitmanch((ID>>16)&0xFFFF); // MSB first
transmitmanch(ID&0xFFFF);
if(HinBye) 
	transmitmanch((mF<<8)|mF); 
else 
	transmitmanch((m0<<8)|m0); // Hi=0xFF, Bye=0x00. 
transmitend(); // always end with the pin LOW
*/
	if (n < 4 * 8) {
		printf("Malformed/too short a datagram.\n");
		return;
	};
	
		
	printf("Expect\tff ff ff ff ff ff ff a5 ID ID XX #  XX=FF or 00\n");
        printf("Result\t.. .. .. .. .. .. ff ");

        for(int i = 0; i < n/8; i++) {
                printf("%02x%s", data[i], (i % 11 == 10) ? "\n\t" : " ");
	};
        printf("\n");

	mainsnode_datagram_t m = *(mainsnode_datagram_t*)data;

	if (m.hdr != MAINSNODE_HDR)
		printf("Fail - wrong heaeder\n");
	else
	if (m.state != MAINSNODE_STATE_ON && m.state != MAINSNODE_STATE_OFF)
		printf("Fail - impossible state value\n");
	else
		printf("\n===>Sensor %08x: %x - state: %s (%x)\n\n", m.val,
			ntohs(m.node_id), (m.state == MAINSNODE_STATE_ON) ? "On" : "Off", m.state);
}

int main(int argc, char ** argv) {
	#define N (5000)
	rmt_data_t vals[N];
	int n = 0;

	int s = sizeof(mainsnode_datagram_t);
	if (s !=4 ) {
		printf("Packing issyes 4 != %d\n", s);
		exit(1);
	};

	if (argc == 2)
		stdin = fopen(argv[1],"r");
	
	while(!feof(stdin) && n < N) {
  		unsigned long delta0, delta1; int val0, val1;
		char buff[1024];
		for(;!feof(stdin);) {
			gets(buff);
			if (sscanf(buff, "%012lu %d", &delta0, &val0))
				break;
			printf("%s\n", buff);
		}
		for(;!feof(stdin);) {
			gets(buff);
			if (sscanf(buff, "%012lu %d", &delta1, &val1))
				break;
			printf("%s\n", buff);
		};
		vals[n++] = (rmt_data_t){ delta0, val0, delta1, val1 };
	};

	printf("Read %d pairs\n", n);
        if (0) for(int i =0; i<n; i++) {
		rmt_data_t p = vals[i];
		printf("-> %d %d\n", p.duration0, p.level0);
		printf("-> %d %d\n", p.duration1, p.level1);
	};

        // Feed things in randomish chunks - just like we'd have
	// in the ESP32 stituation
	for(int i = 0; i < n; ) {
		int chunk_n = 3 + (rand() & 0xFF);
		if (i + chunk_n > n) {
			chunk_n = n - i;
		};
		printf("%d %d %d\n", i, n, chunk_n);
		proc(chunk_n, &(vals[i]));
		i += chunk_n;
	};
};
