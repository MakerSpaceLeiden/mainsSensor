#include "mainsnode.h"
#include <functional>
#include "esp32-hal.h"

#ifndef _H_MAINSENSOR
#define _H_MAINSSENSOR

class MainSensorReceiver {
  public:
    // shall we also pass the PIN, `self' or any void * private data;
    // so that we can deal with multiple listners, etc.
    //
    typedef std::function<void(mainsnode_datagram_t * dgram)> msr_callback_t;
    typedef std::function<void(rmt_data_t * items, size_t len)> msr_raw_callback_t;

    MainSensorReceiver(
      gpio_num_t pin,
      msr_callback_t cb = NULL,
      uint32_t shortPulseMicroSeconds = 250)
      :
      _pin(pin),
      _callback(cb),
      _shortPulseMicroSeconds(shortPulseMicroSeconds)
    {
    }
    void setCallback(msr_callback_t cb) { _callback = cb; };
    void setRawcb(msr_raw_callback_t cb) { _rawcb = cb; };

    void setup(uint32_t shortPulseMicroSeconds = 0);
    void begin(); 
    void end();

    float nanoseconds(uint32_t ticks) { return ticks * _realTickNanoSeconds; };

    // Glue -- not quite public.
    void process(rmt_data_t * items, size_t len);
  private:
    gpio_num_t _pin;
    msr_callback_t _callback = NULL;
    msr_raw_callback_t _rawcb = NULL;
    uint32_t _shortPulseMicroSeconds, _shortPulseTicks, _discriminatorTicks;
    float _realTickNanoSeconds;
    rmt_obj_t* rmt_recv = NULL;
};

#endif
