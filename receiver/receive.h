#include <strings.h>

typedef struct { 
	unsigned long d; 
	int v;
} item_t;

extern void proc_datagram(size_t n, unsigned char * buff);
void proc(size_t n, item_t * data);
