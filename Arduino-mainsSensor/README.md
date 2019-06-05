mainsSensor - Arduino version
=============================

Glue around the lukelectro mainsSensor code to allow use on the
Arduino - currently aimed at the ESP32.

It simply takes the GPIO pin of the 433Mhz receiver module; and
plucks its manchester signal straight from it.

Typical use is with a simple callback to process each (hopefully
valid) frame. Note that there is no CRC; just a bit of manchester
redundancy !

    void setup() {
        ....
        MainSensorReceiver msr = MainSensorReceiver(
          GPIO_9 /* receiver pin */, 
	  (void)(mainsnode_datagram_valid_hdr * node) {
	    uint16_t node_id = node->node_id;
   	    uint8_t state = node->state;
   
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

	void mainsnode_datagram_valid_hdr * node) {
		...
	void setup() {
        	....
                MainSensorReceiver msr = MainSensorReceiver(GPIO_9, &mainsnode_datagram_valid_hdr);

 
