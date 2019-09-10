
// 433 Mhz radio output pint wired to pin GPIO 13.
//
#define GPIO_INPUT_PIN (GPIO_NUM_13)

#include <mainsSensor.h>

MainSensorReceiver msr = MainSensorReceiver(GPIO_INPUT_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  Serial.println("Started" __FILE__" / " __DATE__ " / " __TIME__ );

  pinMode(GPIO_INPUT_PIN, INPUT);
  msr.setup(350 /* 350 milli seconds for a half bit */);
  msr.setCallback([](mainsnode_datagram_t * node) {
    switch (node->state) {
      case MAINSNODE_STATE_ON:
        Serial.printf("Node %04x is on", node->id16);
        break;
      case MAINSNODE_STATE_OFF:
        Serial.printf("Node %04x is OFF", node->id16);
        break;
      default:
        Serial.printf("Node %04x sent a value I do not understand.", node->id16);
    }
  });

  msr.begin();
#if 0
  msr.setRawcb([](rmt_data_t * items, size_t len) {
    Serial.printf("CB: %d items\n", len);
    for (int i = 0; i < len; i++) {
      if (items[i].duration0)
        Serial.printf("%d: L=%d %8d # %12.2f nSecs\n", i * 2,
                      items[i].level0, items[i].duration0, msr.nanoseconds(items[i].duration0));
      if (items[i].duration1)
        Serial.printf("%d: L=%d %8d # %12.2f nSecs\n", i * 2 + 1,
                      items[i].level1, items[i].duration1, msr.nanoseconds(items[i].duration1));
    };
  });
#endif
};

void loop() {
  Serial.println("tock");
  delay(1000); // Sleep 3 seconds between tocks.
};
