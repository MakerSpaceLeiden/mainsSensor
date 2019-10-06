
// 433 Mhz radio output pin 'DATA' wired to pin GPIO 5.
//
#define GPIO_INPUT_PIN (GPIO_NUM_5)

#include <mainsSensor.h>
#include <lwip/def.h> // for htons()

MainSensorReceiver msr = MainSensorReceiver(GPIO_INPUT_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  delay(250);

  Serial.println("Started" __FILE__" / " __DATE__ " / " __TIME__ );

  pinMode(GPIO_INPUT_PIN, INPUT);
  msr.setup(200 /* 200 micro seconds for a half bit */);

  msr.setCallback([](mainsnode_datagram_t * node) {
    static int ok = 0;
    unsigned long secs = 1+millis() / 1000;
    ok++;
    Serial.printf("%06lu:%02lu:%02lu %4.1f%% :\t",
                  secs / 3600, (secs / 60) % 60, secs % 60,  ok * 100. / secs);

    switch (node->state) {
      case MAINSNODE_STATE_ON:
        Serial.printf("Node %04x is on\n", htons(node->id16));
        break;
      case MAINSNODE_STATE_OFF:
        Serial.printf("Node %04x is OFF\n", htons(node->id16));
        break;
      default:
        Serial.printf("Node %04x sent a value %x I do not understand.\n", htons(node->id16), node->state);
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
  delay(3 * 1000); // Sleep 3 seconds between tocks.
};
