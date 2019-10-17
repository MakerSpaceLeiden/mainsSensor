#include "driver/rmt.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <rom/crc.h>
#include <lwip/def.h> // for htons()

#include "Arduino.h"

#include "mainsSensor.h"

#include <strings.h>
#include <list>

#include "mainsnode.h"

#include "avr-crc8.h"

// See:  https://github.com/espressif/arduino-esp32/pull/3345
// #define OLD_STYLE 1

#define IFDebug if (_debug) (*_debug)

#define MS_DEBUG 1

#ifdef MS_FULL_DEBUG
static void _dump(rmt_data_t* items, size_t n_items, int _halfBitTicks, float _realTickNanoSeconds) {
  int D  = 0;
  int d = 0;
  for (int i = 0; i < 2 * n_items;) {
    int duration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
    int level =   ((i % 2) ? items[i >> 1].level1    : items[i >> 1].level0) ? 1 : 0;
    i++;

    if (duration == 0) {
      IFDebug.println(".");
      IFDebug.printf("Halfbit ticks = %d, %f\n", D / d, 1. * D / d / _halfBitTicks);
      return;
    }
    IFDebug.printf("%d", duration);

    char c = level ? '-' : '_';
    if (duration < _halfBitTicks / 5) {
      IFDebug.printf("!");
    } else if (duration < 1.5 * _halfBitTicks) {
      IFDebug.printf("%c.", c);
      d += 1; D += duration;
    } else if (duration > 3 * _halfBitTicks) {
      IFDebug.printf("%c%c%c%c.", c, c, c, c);
      d += 4; D += duration;
    } else if (duration > 1.5 * _halfBitTicks) {
      IFDebug.printf("%c%c.", c, c);
      d += 2; D += duration;
    } else {
      IFDebug.printf("%c?.", c);
      d = 0;
    };
  }
  IFDebug.println("<nol>");
};
#endif

void MainSensorReceiver::process(rmt_data_t* items, size_t n_items)
{
  enum { SEEK, LONGS, READING, RESET } s = SEEK;
  uint8_t out[4] = { 0, 0, 0, 0 };
  int bits_read = 0;
  double lt = 0, f1 = 0 , f2 = 0, mi = 5 * _halfBitMicroSeconds, ma = 0;

  portENTER_CRITICAL_ISR(&queueMux);
  woke++;
  portEXIT_CRITICAL_ISR(&queueMux);

  if (_rawcb)
    _rawcb(items, n_items);

  if (n_items < 22) {
    portENTER_CRITICAL_ISR(&queueMux);
    tooshort++;
    portEXIT_CRITICAL_ISR(&queueMux);
    return;
  };

  for (int i = 0; i < 2 * n_items;) {
    int duration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
    int level =   ((i % 2) ? items[i >> 1].level1    : items[i >> 1].level0) ? 1 : 0;
    i++; // for next duration */

    if (duration == 0)
      break;

    if (duration < glitchShort) {
      portENTER_CRITICAL_ISR(&queueMux);
      glitched++;
      portEXIT_CRITICAL_ISR(&queueMux);
      continue;
    };

    // SKEE for a long high followed by a LONGS low; we
    // can then begin reading (no need to skip first half bit).
    //
    switch (s) {
      case RESET:
        s = SEEK;
        bits_read = 0;
        lt = 0;
      /* NO break */
      case SEEK:
        if (duration > minPreamble && duration < maxPreamble && level == 1) {
          s = LONGS;
	  f1 = duration / 4.0;
        };
        break;
      case LONGS:
        if (duration > minPreamble && duration < maxPreamble && level == 0) {
	  // we do not need anything special here - as all ID's have the top bit
	  // set - so we know that the next one is a low->high transition; i.e. a '1'
	  //
          s = READING;
	  f2 = duration / 4.0;
        } else {
          s = RESET;
        };
        break;
      case READING:
        if ((duration > maxLong || duration < minShort) || ( duration > maxShort && duration < minLong)) {
          s = RESET;
          continue;
        };

        uint32_t d = duration;
        if (duration > minLong) {
		d = duration / 2;
	};
	if (d < mi) mi = d;
	if (d > ma) ma = d;
	lt += d;
        // if this level is high; then we the previous one was low; so
        // we went 'up' (as RMT only passes changes). If it is low
        // we dit a high to low.
	//
        out[bits_read / 8] |= (level ? 1 : 0) << (7 - (bits_read & 7));
        bits_read++;

        // Skip over the next short; if any.
        //
        int nxtduration = (i % 2) ? items[i >> 1].duration1 : items[i >> 1].duration0;
        if (nxtduration < maxShort ) {
          i++;
        };

        if (bits_read == 32) {
          mainsnode_datagram_t * msg = (mainsnode_datagram_t *)out;
          uint8_t crc = avr_crc8_ccitt(msg->raw.payload, sizeof(msg->raw.payload));
          portENTER_CRITICAL_ISR(&queueMux);
          full++;
          portEXIT_CRITICAL_ISR(&queueMux);
          if (crc == msg->raw.crc) {
             portENTER_CRITICAL_ISR(&queueMux);
	     ok++;

	     std::unordered_map<unsigned short, record_t>::const_iterator got = state.find(msg->id16);

             record_t rec;
             if (got != state.end()) {
                   rec = got->second;
              	   rec.lastReported = millis();
             	   if ((rec.msg.state != msg->state) || (_cache == false))
                       rec.lastChanged = millis();
		   rec.msg.state = msg->state;
	     } else {
                   rec = { millis(), millis(), *msg, 0.0, 0.0, 0.0 ,0,0 };
             };
             rec.lt = lt;
             rec.f1 = f1;
             rec.f2 = f2;
             rec.mi = mi;
             rec.ma = ma;

	     state[ msg->id16 ] = rec;

             portEXIT_CRITICAL_ISR(&queueMux);
             s = RESET;
             continue;
          };
          portENTER_CRITICAL_ISR(&queueMux);
	  badcrc++;
          portEXIT_CRITICAL_ISR(&queueMux);
#if MS_DEBUG
          IFDebug.printf("mainsSensor(%d): Bad CRC 0x%x != 0x%x on MSG: 0x%x\n",_pin,
                        crc, msg->raw.crc, htonl(msg->raw32));
#endif
          s = RESET;
        };
        break;
    }; // case
  }; // loop
  return;
}

// extern "C" -- trapoline back to c++.
static void _aggregator_ticker(void *arg)
{
  ((MainSensorReceiver *)arg)->aggregate();
}

void MainSensorReceiver::aggregate() {
       std::list<record_t> lst = {};
        portENTER_CRITICAL_ISR(&queueMux);

        for(auto i : state) {
                record_t rec = i.second;
                if (MAINSNODE_STATE_DEAD == rec.msg.state)
			continue;
		if (millis() - rec.lastChanged > maxAge) {
			rec.lastChanged = millis();
			rec.msg.state = MAINSNODE_STATE_DEAD;
                        state[ i.first ] = rec;
		};
		if ((int64_t)rec.lastChanged - lastAggregation>0) {
			lst.push_back(rec);
		};
	};
        portEXIT_CRITICAL_ISR(&queueMux);
	lastAggregation = millis();

 	if (_callback)
		for (record_t rec: lst)  {
			_callback(&(rec.msg));

	IFDebug.printf("mainsSensor(%d): Halfbit %.1f uSecond, %3.1f %% off\n", _pin,
		_realTickNanoSeconds * rec.lt / 32 / 1000., 
		_realTickNanoSeconds * rec.lt / 32 / 10. / _halfBitMicroSeconds - 100.0);
	IFDebug.printf("mainsSensor(%d): timings: %.1f/%.1f vs. %.1f (%.1f%%)< %.1f < %.1f (%.1f%%)\n", _pin,
		rec.f1 * _realTickNanoSeconds/1000.,
		rec.f2 * _realTickNanoSeconds/1000.,
		1.* rec.mi * _realTickNanoSeconds/1000., 
		100.*(1. * rec.mi - 1. *_halfBitTicks) / _halfBitTicks,
		1.* rec.lt / 32 * _realTickNanoSeconds/1000., 
		1.* rec.ma * _realTickNanoSeconds/1000.,
		100.*(1. * rec.ma - 1. *_halfBitTicks) /_halfBitTicks
		);
	};
   if (millis() - lastReport > reportRate * 1000) {
        lastReport = millis();
        portENTER_CRITICAL_ISR(&queueMux);
	IFDebug.printf("mainsSensor(%d): entry: %u, Too short: %u, Considered: %u, Glitches: %u, Complete: %u=(%u ok + %u badcrc),\n",
		_pin,
		woke, tooshort, woke-tooshort, glitched, full, ok, badcrc);
        woke = glitched =  tooshort =  full =  ok = badcrc = 0;
        portEXIT_CRITICAL_ISR(&queueMux);
   }    
};

#ifdef OLD_STYLE
static MainSensorReceiver * _hidden_global = NULL;
static void _receive_data(uint32_t *data, size_t len)
{
  _hidden_global->process((rmt_data_t*)data, len);
}
#else
static void _receive_data(uint32_t *data, size_t len, void * arg)
{
#if 0
  _hidden_global->process((rmt_data_t*)data, len);
  static int i = 0;
  if (i == 0 && arg != _hidden_global) {
	IFDebug.println("Not identical - odd.");
	i++;
	};
#endif
  ((MainSensorReceiver *)arg)->process((rmt_data_t*)data, len);
}
#endif

void MainSensorReceiver::setup(uint32_t halfBitMicroSeconds, uint32_t halfBitPreambleMicroSeconds)
{
#ifdef OLD_STYLE
  _hidden_global = this;
#endif

  if (halfBitMicroSeconds)
    _halfBitMicroSeconds = halfBitMicroSeconds;
  _halfBitPreambleMicroSeconds = halfBitPreambleMicroSeconds ? halfBitPreambleMicroSeconds : halfBitMicroSeconds;

  rmt_recv = rmtInit(_pin, false, RMT_MEM_256);
  // about 32 ticks for a short pulse. To get some meaningful resolution.
  //
  // According to https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/rmt.html
  // image 'Structure of RMT items (L - signal level)' the time has 15 bits. 
   //
  // We range a factor 4-8 around our half bit (2-3 bits) if the preamble/main are not too far of each other.
  //
  // So a decent resolution is 15 - 3 = 12 - so say: 10 bits = 1024.
  //
  double maxlen = _halfBitMicroSeconds; if (maxlen < _halfBitMicroSeconds) maxlen = _halfBitMicroSeconds;
  _realTickNanoSeconds = rmtSetTick(rmt_recv, maxlen * 1000 / 512 /* nano Seconds */);
  _halfBitTicks = 1000 * _halfBitMicroSeconds / _realTickNanoSeconds;

  uint32_t minTicks = (_halfBitMicroSeconds * 1000. / 2) / _realTickNanoSeconds - 1;
  uint32_t maxTicks = (_halfBitMicroSeconds * 1000. * 8) / _realTickNanoSeconds + 1;

  // In receive mode, ignore input pulse when the pulse width is smaller than threshold. 
  // Counted in source clock, not divided counter clock. (esp_err_t rmt_set_rx_filter())
  // XXX not correcting for clock type yet.
  rmtSetFilter(rmt_recv, true, minTicks > 254 ? 254 : minTicks);

  // In receive mode, when no edge is detected on the input signal for longer than 
  // idle_thres channel clock cycles, the receive process is finished. (rmt_set_rx_idle_thresh)
  rmtSetRxThreshold(rmt_recv, maxTicks);

  // Shorter than this - we ignore. Which is odd - as the SetFilter should
  // stop us seeing this.
  glitchShort = minTicks - 5; // 0.1 * _halfBitTicks;

  // Slant towards the bottom of the range; as the halfbits
  // during the data tend to be 60% longer than those in the
  // pre-amble.
  minPreamble = 3 *  1000 * _halfBitPreambleMicroSeconds / _realTickNanoSeconds;
  maxPreamble = 5 *  1000 * _halfBitPreambleMicroSeconds / _realTickNanoSeconds;

#define off (0.5) /* 25 % */
  minShort =  (1.-off) * _halfBitTicks;
  maxShort =  (1.+off) * _halfBitTicks;
  minLong =   (2.-off) * _halfBitTicks;
  maxLong =   (2.+off) * _halfBitTicks;

#if MS_DEBUG
  IFDebug.printf("mainsSensor: pin %d\n", _pin);
  IFDebug.printf(" half bit:  %12d microSeconds\n", _halfBitMicroSeconds);
  IFDebug.printf("            %12.1f nanoSeconds\n", 1000. * _halfBitMicroSeconds);
  IFDebug.printf(" Tick:      %12.1f nanoSeconds\n", _realTickNanoSeconds);
  IFDebug.printf(" halfbit:   %12u #\n", _halfBitTicks);
  IFDebug.printf(" min:       %12u #\n", minTicks);
  IFDebug.printf(" max:       %12u #\n", maxTicks);
#endif
  woke = glitched =  tooshort =  full =  ok = badcrc = 0;
};

void MainSensorReceiver::begin() {
#ifdef OLD_STYLE
  rmtRead(rmt_recv, _receive_data);
#else
  rmtRead(rmt_recv, _receive_data, this);
#endif
  aggregator.attach_ms(333, _aggregator_ticker, (void *)this);   
}

void MainSensorReceiver::end() {
#ifdef OLD_STYLE
  assert("not implemented" == NULL);
#else
  rmtEnd(rmt_recv);
#endif
  aggregator.detach();
};

MainSensorReceiver::~MainSensorReceiver() {
#ifndef OLD_STYLE
  end();
  rmtDeinit(rmt_recv);
#endif
}

void MainSensorReceiver::setCache(bool onOff) { _cache = onOff; }

