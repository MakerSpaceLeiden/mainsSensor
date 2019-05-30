#include <readline/readline.h>
#include <strings.h>


typedef struct { 
	unsigned long d; 
	int v;
} item_t;

unsigned char rev8(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

void proc_datagram(int n, unsigned char * data) {
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
	printf("Read %d bits - expected 11x8=88 bits\n", n);
	printf("Expect\tff ff ff ff ff ff ff a5 ID ID XX #  XX=FF or 00\n");
	printf("Expect\tff ff ff ff ff ff ff a5 77 77 XX\n");
        printf("Result\t.. .. .. .. .. .. ");

        for(int i = 0; i < n/8; i++) {
                printf("%02x%s", rev8(data[i]), (i % 11 == 10) ? "\n\t" : " ");
	};
        printf("\n");
}

int tick = 100;
void proc(size_t n, item_t * data) {
	#define SHORT 0
	#define LONG 1
	#define HIGH 1
	#define LOW 0
	unsigned char out[512];
	int j = 0;
	int tickcrit = 1.5 * tick;
	enum { TRAIN, SYNC, READ } state;
	int len = 0;
	for(int i = 0; i < n-1; i++) {
		if (data[i].d > 3* tickcrit) {
			printf("Sta to train (gap)\n");
			i++;
			state = TRAIN;
			len = 0;
			continue;
		};
		if (state == TRAIN) {
			// Keep going until we see at least 9 short pulses
			// and no long one.
			if (data[i].d < tickcrit || len < 24) {
				len++;
				continue;
			};
			state = SYNC;
			printf("State to sync (%d FF bytes)\n", len/16);
			bzero(out,sizeof(out));
			for(j=0;j<9;j++)
				out[j/8] |= (1<<(j&7));
		}; 
		
		out[j/8] |= (data[i].v ? 0 : 1)<<(j & 7);
		j++;
		if (data[i+1].d < tickcrit)
			i++;

		if (state == SYNC) {
			if (j == 16) {
				// Seek for ff A5
				state = (out[1] == 0xA5) ? READ : SYNC;
				printf("State to %s\n", state == READ ? "read" : "sync");
			};
			continue;
		};
		if (state == READ && j == 40) {
			proc_datagram(j, out);
			printf("State to Train (end)\n");
			state = TRAIN;
		};
	};
}

int main(int argc, char ** argv) {
  	unsigned long delta;
	int val;
	#define N (5000)
	item_t vals[N];
	int n = 0;
	
	while((!feof(stdin)) && n < N) {
		char buff[1024];
		gets(buff);
		if (sscanf(buff, "%012lu %d", &delta, &val)) {
			vals[n++] = (item_t){ delta, val };
			// printf("-->%d %lu %d\n", n, delta, val);
		} else {
			printf("%s\n", buff);
		};
	};
	proc(n, vals);
};
