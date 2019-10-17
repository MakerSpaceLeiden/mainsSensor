#include "mainsnode.h"
#include <functional>
#include "esp32-hal.h"
#include <Ticker.h>
#include <unordered_map>

#ifndef _H_MAINSENSOR
#define _H_MAINSSENSOR

class MainSensorReceiver {
  public:
    // shall we also pass the PIN, `self' or any void * private data;
    // so that we can deal with multiple listners, etc.
    //
    typedef std::function<void(mainsnode_datagram_t * dgram)> msr_callback_t;
    typedef std::function<void(rmt_data_t * items, size_t len)> msr_raw_callback_t;

    typedef struct { 
	unsigned long lastReported, lastChanged; 
	mainsnode_datagram_t msg; 
	double lt; 
	double f1, f2;
	uint32_t mi, ma; 
     } record_t;

    MainSensorReceiver(
      gpio_num_t pin,
      msr_callback_t cb = NULL,
      uint32_t halfBitMicroSeconds = 250)
      :
      _pin(pin),
      _callback(cb),
      _halfBitMicroSeconds(halfBitMicroSeconds)
    {
    }
    ~MainSensorReceiver();

    void setDebug(Print * debugPrint) {
       _debug = debugPrint;
    };

    void setCallback(msr_callback_t cb) {
      _callback = cb;
    };
    void setRawcb(msr_raw_callback_t cb) {
      _rawcb = cb;
    };

    void setup(uint32_t halfBitMicroSeconds = 0, uint32_t halfBitPreambleMicroSeconds = 0);
    void begin();
    void end();
    void setCache(bool onOff);

    float nanoseconds(uint32_t ticks) {
      return ticks * _realTickNanoSeconds;
    };

    // Glue -- not quite public. - trampoline only
    void process(rmt_data_t * items, size_t len);
    void aggregate();
  private:
    bool _cache;
    Print * _debug = NULL;

    gpio_num_t _pin;

    msr_callback_t _callback = NULL;
    msr_raw_callback_t _rawcb = NULL;

    uint32_t _halfBitMicroSeconds, _halfBitTicks, _discriminatorTicks, _halfBitPreambleMicroSeconds;
    float _realTickNanoSeconds;

    uint32_t tooshort, glitched, badcrc, ok, full, woke;
    uint32_t lastReport, reportRate = 30; // every 30 seconds.
    rmt_obj_t* rmt_recv = NULL;

    portMUX_TYPE queueMux = portMUX_INITIALIZER_UNLOCKED;

    // Keep a table of all known nodes and their state.
    Ticker aggregator;
    std::unordered_map<unsigned short, record_t> state;
    unsigned long lastAggregation;
    unsigned long maxAge = MAINSNODE_MAXAGE;

    uint32_t glitchShort ;
    uint32_t minPreamble ;
    uint32_t maxPreamble ;
    uint32_t minShort ;
    uint32_t maxShort ;
    uint32_t minLong ;
    uint32_t maxLong ;
};
#endif
