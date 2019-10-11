mainsSensor - Arduino version
=============================

Glue around the lukelectro mainsSensor code to allow use on the
Arduino - currently aimed at the ESP32.

It simply takes the GPIO pin of the 433Mhz receiver module; and
plucks its manchester signal straight from it.

Typical use is with a simple callback to process each (hopefully
valid) frame.

    void setup() {
        ....
        MainSensorReceiver msr = MainSensorReceiver(
          GPIO_9 /* receiver pin */, 
	  (void)(mainsnode_datagram_valid_hdr * telegram) {
	    uint16_t node_id = telegram->node_id;
   	    uint8_t state = telegram->state;
   
            switch(state) {
            case MAINSNODE_STATE_ON:
   		// ... node with ID node_id is on
   		break;
            case MAINSNODE_STATE_OFF:
   		// ... node with ID node_id is off (i.e. just gone off).
   		break;
      	    default:
		// .. unknown state.
      	    };
          }
        );


Or more traditional:

	void process(mainsnode_datagram_valid_hdr * msg) {
		do stuff...
	void setup() {
        	....
                MainSensorReceiver msr = MainSensorReceiver(GPIO_9, &process);


## Design note

On  the EPS32 the library uses the hardware based RMT facility to record the pulsetrain with a bit of pre-filtering. Valid
datagrams are then placed on a processing queue (as the RM callback should return relatively swiftly; and should not do
things like extensive Serial comms).

This queue is then checked 2-3 times per second by a ticker; which will process the *most recent* datagram of a given
node (as nodes commonly send short burts of them). And it will then only call the callback when the state of a datagram
has changed (was turned on or off); or when it did not report in frequently enough (DEAD).

## CRC

There is a simple CRC8 checksum - this works - but in test we saw about 1 or 2 frames getting through a week that
are 'impossible'.
