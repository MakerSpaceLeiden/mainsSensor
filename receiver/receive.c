#include <readline/readline.h>
#include <strings.h>
#include "receive.h"

#define tick 100
int tickcrit = 1.5 * tick;

void proc(size_t n, rmt_data_t * data) {
	unsigned char out[40];
	int j = 0;
	enum { TRAIN, SYNC, READ } state;
	int len = 0;
	for(int i = 0; i < 2*n-1;) {
		int duration = (i % 2) ? data[i>>1].duration1 : data[i>>1].duration0;
		int level =    (i % 2) ? data[i>>1].level1    : data[i>>1].level0;
		i++;
		int nxtduration = (i % 2) ? data[i>>1].duration1 : data[i>>1].duration0;

		if (duration > 3* tickcrit) {
			i++;
			state = TRAIN;
			len = 0;
			continue;
		};
		if (state == TRAIN) {
			// Keep going until we see at least 9 short pulses
			// and no long one.
			if (duration < tickcrit || len < 24) {
				len++;
				continue;
			};
			state = SYNC;
			bzero(out,sizeof(out));
			out[0] = 0xFF; out[1] = 0x80; j = 9;
		}; 

		out[j/8] |= (level ? 0 : 1)<<(7-(j & 7));
		j++;
		if (nxtduration < tickcrit) {
			i++;
		};

		if (state == SYNC) {
			if (j == 16) {
				// Seek for ff A5
				state = (out[1] == 0xA5) ? READ : TRAIN;
			};
			continue;
		};
		if (state == READ && j == 40) {
			proc_datagram(j, out);
			state = TRAIN;
		};
	};
}
