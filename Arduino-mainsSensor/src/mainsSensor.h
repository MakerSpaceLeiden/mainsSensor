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

    typedef struct { unsigned long lastReported, lastChanged; messages_t state; } record_t;

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

    void setCallback(msr_callback_t cb) {
      _callback = cb;
    };
    void setRawcb(msr_raw_callback_t cb) {
      _rawcb = cb;
    };

    void setup(uint32_t halfBitMicroSeconds = 0);
    void begin();
    void end();

    float nanoseconds(uint32_t ticks) {
      return ticks * _realTickNanoSeconds;
    };

    // Glue -- not quite public. - trampoline only
    void process(rmt_data_t * items, size_t len);
    void aggregate();
  private:
    gpio_num_t _pin;
    msr_callback_t _callback = NULL;
    msr_raw_callback_t _rawcb = NULL;
    uint32_t _halfBitMicroSeconds, _halfBitTicks, _discriminatorTicks;
    float _realTickNanoSeconds;
    rmt_obj_t* rmt_recv = NULL;

    // Queue onto which we put up to N datagrams; to be
    // aggregated every 200-300 milli second or so.
    const int maxQueue = 16;
    mainsnode_datagram_t queue[ 16 ];
    int nQueued = 0;
    portMUX_TYPE queueMux = portMUX_INITIALIZER_UNLOCKED;

    // Keep a table of all known nodes and their state.
    Ticker aggregator;
    std::unordered_map<unsigned short, record_t> state;
};
#endif
