#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "transmit.h"

// const uint32_t ID = 0x6A6A6A6A; //0x7777
const uint32_t	  ID = 0x12345678; //0x7777

static unsigned long timecntr = 0; //microSeconds

void
_transmit(int level)
{
	static unsigned int clevel = 2;

	if (level == clevel) 
		return;

	static unsigned long last = 0;
	unsigned long	delta = timecntr - last;

	printf("%012lu %d\n", delta, level);

	last = timecntr;
	clevel = level;
};

void
transmitend()
{
	_transmit(0);
};

void
transmitmanch(uint16_t tx)
{
	uint8_t		i = 0;
	printf("# %012lu: %04x\n", timecntr, tx);

	do {
		int level = 0;
		if (tx & 0x8000)
			level = 1;
		_transmit(level);
		i++;
		tx = tx << 1; //MSB first
		timecntr += 100 + (rand() & 3); //microSeconds
	} while (i < 16);
}

int
main(void)
{
	for (int i = 0; i < 10; i++) {
		printf("# Start of transmision %d -- device is %s\n", i + 1, (i < 8) ? "on" : "off");
		transmitframe((i < 8) ? 1 : 0);
		printf("# End of transmission %d --- sleeping\n", i + 1);

		timecntr += 2000000 + (rand() & 255);
                
	}
	close(stdout);
	exit(0);
}
