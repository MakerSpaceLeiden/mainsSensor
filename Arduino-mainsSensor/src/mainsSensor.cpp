#include "driver/rmt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <rom/crc.h>

#include "Arduino.h"

#include "mainsSensor.h"

#include <strings.h>
#include "mainsnode.h"

#include "avr-crc8.h"

static void _dump(rmt_data_t* items, size_t n_items, int _halfBitTicks, float _realTickNanoSeconds) {
  int D  = 0;
  int d = 0;
  for (int i = 0; i < 2 * n_items;) {
    int duration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
    int level =   ((i % 2) ? items[i >> 1].level1    : items[i >> 1].level0) ? 1 : 0;
    i++;

    if (duration == 0) {
      Serial.println(".");
      Serial.printf("Halfbit ticks = %d, %f\n", D / d, 1. * D / d / _halfBitTicks);
      return;
    }
    Serial.printf("%d", duration);

    char c = level ? '-' : '_';
    if (duration < _halfBitTicks / 5) {
      Serial.printf("!");
    } else if (duration < 1.5 * _halfBitTicks) {
      Serial.printf("%c.", c);
      d += 1; D += duration;
    } else if (duration > 3 * _halfBitTicks) {
      Serial.printf("%c%c%c%c.", c, c, c, c);
      d += 4; D += duration;
    } else if (duration > 1.5 * _halfBitTicks) {
      Serial.printf("%c%c.", c, c);
      d += 2; D += duration;
    } else {
      Serial.printf("%c?.", c);
      d = 0;
    };
  }
  Serial.println("<nol>");
};

void MainSensorReceiver::process(rmt_data_t* items, size_t n_items)
{
  enum { SEEK, LONGS, READING, RESET } state = SEEK;
  uint8_t out[32] = { 0, 0, 0, 0 };
  int bits_read = 0;

  if (_rawcb)
    _rawcb(items, n_items);

  if (n_items < 22)
    return;

  for (int i = 0; i < 2 * n_items;) {
    int duration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
    int level =   ((i % 2) ? items[i >> 1].level1    : items[i >> 1].level0) ? 1 : 0;
    i++; // for next duration */

    if (duration == 0)
      break;

    if (duration < _halfBitTicks / 5)
      continue;

    // SKEE for a long high followed by a LONGS low; we
    // can then begin reading (no need to skip first half bit).
    //
    switch (state) {
      case RESET:
        state = SEEK;
        bits_read = 0;
      /* NO break */
      case SEEK:
        if (duration > 3 * _halfBitTicks && level == 1)
          state = LONGS;
        break;
      case LONGS:
        if (duration > 3 * _halfBitTicks  && duration < 5 * _halfBitTicks && level == 0) {
          // _dump(items, n_items, _halfBitTicks, _realTickNanoSeconds);
          state = READING;
        } else {
          state = RESET;
        };
        break;
      case READING:
        if (duration > 4 * _halfBitTicks) {
          state = RESET;
          continue;
        };
        out[bits_read / 8] |= (level ? 1 : 0) << (7 - (bits_read & 7));
        bits_read++;


        if (bits_read == 32) {
          mainsnode_datagram_t * msg = (mainsnode_datagram_t *)out;
          uint8_t crc = avr_crc8_ccitt(msg->raw.payload, sizeof(msg->raw.payload));

          if (crc == msg->raw.crc) {
            _callback(msg);
            return;
          }
#if MS_DEBUG
          Serial.printf("Bad CRC 0x%x != 0x%x on MSG: 0x%x\n",
                        crc, msg->raw.crc, msg->raw32);
#endif
          state = RESET;
          break;
        };

        // Skip over the next short.
        //
        int nxtduration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
        if (nxtduration < 1.5 * _halfBitTicks) {
          i++;
        };
        break;
    };
  };
  return;
}

#ifdef OLD_STYLE
static MainSensorReceiver * _hidden_global = NULL;
#endif

// extern "C" -- trapoline back to c++.
#ifdef OLD_STYLE
static void _receive_data(uint32_t *data, size_t len)
{
  _hidden_global->process((rmt_data_t*)data, len);
}
#else
static void _receive_data(uint32_t *data, size_t len, void * arg)
{
  ((MainSensorReceiver *)arg)->process((rmt_data_t*)data, len);
}
#endif

void MainSensorReceiver::setup(uint32_t halfBitMicroSeconds)
{
#ifdef OLD_STYLE
  _hidden_global = this;
#endif

  if (halfBitMicroSeconds)
    _halfBitMicroSeconds = halfBitMicroSeconds;

  rmt_recv = rmtInit(_pin, false, RMT_MEM_256);
  // about 32 ticks for a short pulse. To get some meaningful resolution.
  //
  _realTickNanoSeconds = rmtSetTick(rmt_recv, _halfBitMicroSeconds * 1000 / 32 /* nano Seconds */);
  _halfBitTicks = 1000 * _halfBitMicroSeconds / _realTickNanoSeconds;

  // Well larger than _delay_us(HALFBITTIME);
  uint32_t minTicks = (_halfBitMicroSeconds * 1000. / 2) / _realTickNanoSeconds - 1;
  rmtSetFilter(rmt_recv, true, minTicks);

  // Well larger than _delay_us(HALFBITTIME*4);
  uint32_t maxTicks = (_halfBitMicroSeconds * 1000. * 6) / _realTickNanoSeconds + 1;
  rmtSetRxThreshold(rmt_recv, maxTicks);

#if MS_DEBUG
  Serial.printf("half bit:  %12d   microSeconds\n", _halfBitMicroSeconds);
  Serial.printf("           %12.1f nanoSeconds\n", 1000. * _halfBitMicroSeconds);
  Serial.printf("Tick:      %12.1f nanoSeconds\n", _realTickNanoSeconds);
  Serial.printf("halfbit:   %12u   #\n", _halfBitTicks);
  Serial.printf("min:       %12u   #\n", minTicks);
  Serial.printf("max:       %12u   #\n", maxTicks);
#endif
};

void MainSensorReceiver::begin() {
#ifdef OLD_STYLE
  rmtRead(rmt_recv, _receive_data);
#else
  rmtRead(rmt_recv, _receive_data, this);
#endif
}

void MainSensorReceiver::end() {
#ifdef OLD_STYLE
  assert("not implemented" == NULL);
#else
  rmtEnd(rmt_recv);
#endif
};

MainSensorReceiver::~MainSensorReceiver() {
#ifndef OLD_STYLE
  end();
  rmtDeinit(rmt_recv);
#endif
}
