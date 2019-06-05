#include "receive.h"
#include "mainsnode.h"
#include <functional>

class MainSensorReceiver {
public:
	// shall we also pass the PIN, `self' or any void * private data;
        // so that we can deal with multiple listners, etc.
	//
        typedef std::function<void(mainsnode_datagram_t * dgram)> msr_callback_t;

	MainSensorReceiver(
           gpio_num_t pin,
           msr_callback_t cb = NULL,
           rmt_channel_t rmt_channel = RMT_CHANNEL_0,
           uint32_t shortPulseMicroSeconds = 100) 
        : 
		_pin(pin),
		_callback(cb),
		_rmt_channel(rmt_channel),
		_shortPulseMicroSeconds(shortPulseMicroSeconds)
	{
	}

       void setup(); 
       void begin(); // needs to be called to stop colleting data.
       void end();

       // Glue -- not quite public.
       void update(mainsnode_datagram_t * dgram);
       gpio_num_t _pin;
       msr_callback_t _callback;
       rmt_channel_t _rmt_channel;
       uint32_t _shortPulseMicroSeconds;
}

;
