#include "driver/rmt.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "Arduino.h"

#include "mainsSensor.h"

#include <strings.h>
#include "mainsnode.h"

// The ESP32 standard CRC16 seems to be the CIIT type; while the
// AVR crc16 sees to be 0xA001.
//
static unsigned int crc16_update(unsigned int crc, unsigned char a)
{
    int i;
    crc ^= a;
    for (i = 0; i < 8; ++i)
    {
        if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
        else
        crc = (crc >> 1);
    }
    return crc;
}

void MainSensorReceiver::process(rmt_data_t* items, size_t n_items)
{
  enum { SEEK, LONGS, READING } state = SEEK;
  uint8_t out[32];
  int bits_read = 0;

  if (_rawcb)
	_rawcb(items,n_items);

  for (int i = 0;i<2*n_items;) {
    int duration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
    int level =   ((i % 2) ? items[i >> 1].level1    : items[i >> 1].level0) ? 1 : 0;
    i++; // for next duration */

    if (duration == 0)
      break;

    // can be done much tighter.
    if (n_items - i < 30 - bits_read)
      break;

    switch(state) {
	case SEEK:
      	  if (duration > _shortPulseTicks * 3 && level == 1)
		state = LONGS;
          break;
        case LONGS: 
      	  if (duration > _shortPulseTicks * 3 && level == 0) {
		state = READING; 
                bits_read = 0;
	  } else {
		state = SEEK;
	  };
	  break;
        case READING:
           out[bits_read / 8] |= (level ? 0 : 1) << (7 - (bits_read & 7));
           bits_read++;

           int nxtduration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
           if (nxtduration < _shortPulseTicks * 3) 
              i++;

           if (bits_read == 32) {
              mainsnode_datagram_t * msg = (mainsnode_datagram_t *)out;

  	      uint32_t crc = 0;
              for(int k = 0; k < sizeof(msg->raw.payload); k++) 
                 crc = crc16_update(crc, msg->raw.payload[k]);

              if (crc == msg->raw.crc)
		_callback(msg);
              state = SEEK;
           };
	   break;
        };
     };
  return;
}

static MainSensorReceiver * _hidden_global = NULL;

extern "C" void receive_data(uint32_t *data, size_t len)
{
  _hidden_global->process((rmt_data_t*)data, len);
}

void MainSensorReceiver::setup()
{
  _hidden_global = this;
  rmt_recv = rmtInit(_pin, false, RMT_MEM_256);

  // about 32 ticks for a short pulse. To get some meaningful resolution. 
  // 
  _realTickNanoSeconds = rmtSetTick(rmt_recv, _shortPulseMicroSeconds * 1000 / 32 /* nano Seconds */);
  _shortPulseTicks = 1000 * _shortPulseMicroSeconds / _realTickNanoSeconds;

  // Well larger than _delay_us(HALFBITTIME);
  uint32_t minTicks = (_shortPulseMicroSeconds * 1000. / 2) / _realTickNanoSeconds - 1;

  // Well larger than _delay_us(HALFBITTIME*4);
  uint32_t maxTicks = (_shortPulseMicroSeconds * 1000. * 6) / _realTickNanoSeconds + 1;
  rmtSetFilter(rmt_recv, true, minTicks);
  rmtSetRxThreshold(rmt_recv, maxTicks);

#if 0
  Serial.printf("half bit:  %12d microSeconds\n", _shortPulseMicroSeconds);
  Serial.printf("           %12.2f nanoSeconds\n", 1000. * _shortPulseMicroSeconds);
  Serial.printf("Tick:      %12.2f nanoSeconds\n", _realTickNanoSeconds);
  Serial.printf("halfbit:   %12u #\n", _shortPulseTicks);
  Serial.printf("min:       %12u #\n", minTicks);
  Serial.printf("max:       %12u #\n", maxTicks);
#endif
};

void MainSensorReceiver::begin() {
  rmtRead(rmt_recv, receive_data);
}

void MainSensorReceiver::end() {
  // rmtDeinit(rmt_recv);
}
