#ifndef _H_RECEIVE
#define _H_RECEIVE

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
} rmt_item32_t;
#else
#include "driver/rmt.h"
#include "driver/gpio.h"
#include "Arduino.h"
#endif

typedef void (*datareceived_callback_t)(unsigned char * data, size_t bits_received, void * extra_cb_arg);

void proc(volatile rmt_item32_t * data, datareceived_callback_t callback,  void * extra_cb_arg); /* 0,0 terminated */
#endif
