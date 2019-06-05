
// 433 Mhz radio output pint wired to pin GPIO 13.
//
#define GPIO_INPUT_PIN (GPIO_NUM_13)

#include <mainsSensor.h>

MainSensorReceiver msr = MainSensorReceiver(
                           GPIO_INPUT_PIN,
[](mainsnode_datagram_t * node) {
  switch (node->state) {
    case MAINSNODE_STATE_ON:
      Serial.printf("Node %04x is on", node->node_id);
      break;
    case MAINSNODE_STATE_OFF:
      Serial.printf("Node %04x is OFF", node->node_id);
      break;
    default:
      Serial.printf("Node %04x sent a value I do not understand.", node->node_id);
  }
});

void setup() {
  Serial.begin(115200);
  while (!Serial) {};
  Serial.println("Started" __FILE__" / " __DATE__ " / " __TIME__ );

  pinMode(GPIO_INPUT_PIN, INPUT);
  msr.setup();
  msr.begin();
};

void loop() {
  Serial.println("tock");
  delay(1000); // Sleep 3 seconds between tocks.
};
