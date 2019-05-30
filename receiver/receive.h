#include <strings.h>

#ifdef TEST
#include <stdio.h>
#include <sys/kernel_types.h>
// ESP32 structure returned from RMT.
// https://github.com/espressif/esp-idf/blob/ad3b820e7/components/soc/esp32/include/soc/rmt_struct.h
typedef struct {
    union {
        struct {
            uint32_t duration0 :15;
            uint32_t level0 :1;
            uint32_t duration1 :15;
            uint32_t level1 :1;
        };
        uint32_t val;
    };
} rmt_data_t;
#endif

extern void proc_datagram(size_t n, unsigned char * buff);
void proc(size_t n, rmt_data_t * data);
