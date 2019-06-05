#include "driver/rmt.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "Arduino.h"

#include "mainsSensor.h"
#include "receive.c"

static void IRAM_ATTR rmt_data_handler(unsigned char * data, size_t bits_len, void * arg) {
	MainSensorReceiver* c = (MainSensorReceiver*)arg;

	Serial.println("Ack in rmt_data_handler");
	return;

	for(rmt_item32_t * item = (rmt_item32_t *)data; bits_len >= 32; bits_len -=32, item++) 
		c->_callback((mainsnode_datagram_t *)item);
}

static void IRAM_ATTR rmt_isr_handler(void * arg) {
        MainSensorReceiver* c = (MainSensorReceiver*)arg;
	//uint32_t intr_st = RMT.int_st.val;

	// Following 2 lines cargo-culting from
	// https://esp32.com/viewtopic.php?t=7116#p32383
	//
	RMT.conf_ch[c->_rmt_channel].conf1.rx_en = 0;
	RMT.conf_ch[c->_rmt_channel].conf1.mem_owner = RMT_MEM_OWNER_TX;

        volatile rmt_item32_t* items = RMTMEM.chan[c->_rmt_channel].data32;

	// feed what we got (assuming it is 0,0 terminated) to the
	// statefull parser. It will return a packet if it could
	// extract one. Or null if it is done.
	//
	Serial.println("proc");
	proc(items, &rmt_data_handler, arg);

	// Following 2 lines cargo-culting from
	// https://esp32.com/viewtopic.php?t=7116#p32383
	//
 	RMT.conf_ch[c->_rmt_channel].conf1.mem_wr_rst = 1;
	RMT.conf_ch[c->_rmt_channel].conf1.mem_owner = RMT_MEM_OWNER_RX;
	RMT.conf_ch[c->_rmt_channel].conf1.rx_en = 1;

	RMT.int_clr.val |= 3 * c->_rmt_channel;
  	//RMT.int_clr.val = intr_st;
}

void MainSensorReceiver::setup() 
{
  // In theorie is een half bit ongeveer 180 us (een heel bit dus 360). In de praktijk iets 
  // langer omdat de instructies zelf ook tijd kosten. Meer richting 200 (half bit) 
  // respecitievelijk 400 us.
  //
  #define CHL_DIV 80*3 // Assume default clock to be 80 Mhz 

  #define PULSE_IGNORE 100/3  // micro seconds
  #define PULSE_DONE   600/3  // micro second

  rmt_config_t config;

  config.rmt_mode = RMT_MODE_RX;
  config.channel = _rmt_channel;
  config.gpio_num = _pin;
  config.mem_block_num = 8;
  config.clk_div = CHL_DIV;

  config.rx_config.filter_en = true;
  config.rx_config.filter_ticks_thresh = PULSE_IGNORE;
  config.rx_config.idle_threshold =  PULSE_DONE;

  ESP_ERROR_CHECK(rmt_config(&config));

  // low level / priority
  ESP_ERROR_CHECK(rmt_isr_register(rmt_isr_handler, this, ESP_INTR_FLAG_LEVEL1, 0));
  ESP_ERROR_CHECK(rmt_set_rx_intr_en(_rmt_channel,true));
};

void MainSensorReceiver::begin() {
  ESP_ERROR_CHECK(rmt_rx_start(_rmt_channel, true));
}

void MainSensorReceiver::end() {
  ESP_ERROR_CHECK(rmt_rx_stop(_rmt_channel));
}
