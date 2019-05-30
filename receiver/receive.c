#include <readline/readline.h>
#include <strings.h>
#include "receive.h"

#define tick 100
int tickcrit = 1.5 * tick;

void proc(size_t n, item_t * data) {
	unsigned char out[40];
	int j = 0;
	enum { TRAIN, SYNC, READ } state;
	int len = 0;
	for(int i = 0; i < n-1; i++) {
		if (data[i].d > 3* tickcrit) {
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
			bzero(out,sizeof(out));
			// for(j=0;j<9;j++) out[j/8] |= (1<<(j&7));
			out[0] = 0xFF; out[1] = 0x01; j = 9;
		}; 
		
		out[j/8] |= (data[i].v ? 0 : 1)<<(j & 7);
		j++;
		if (data[i+1].d < tickcrit)
			i++;

		if (state == SYNC) {
			if (j == 16) {
				// Seek for ff A5
				state = (out[1] == 0xA5) ? READ : SYNC;
			};
			continue;
		};
		if (state == READ && j == 40) {
			proc_datagram(j, out);
			state = TRAIN;
		};
	};
}

