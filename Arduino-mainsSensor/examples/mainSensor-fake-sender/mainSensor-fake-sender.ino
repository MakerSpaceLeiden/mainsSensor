#include <assert.h>

const int pin = 5;
const  int halfbit = 200; // microSeconds -- but slower in reality.

typedef struct {
  int level;
  int duration;
} rec_t;

static rec_t preamble[2] = { { 1, 4, },  { 0, 4, }, }; // Preamble

// normal manchester
static rec_t nibble2playlist[16][8] = {
  {/* B_0 0b01010101 */  {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1} },
  {/* B_1 0b01010110 */  {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1} },
  {/* B_2 0b01011001 */  {0, 1}, {1, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1} },
  {/* B_3 0b01011010 */  {0, 1}, {1, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1} },
  {/* B_4 0b01100101 */  {0, 1}, {1, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1} },
  {/* B_5 0b01100110 */  {0, 1}, {1, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1} },
  {/* B_6 0b01101001 */  {0, 1}, {1, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1} },
  {/* B_7 0b01101010 */  {0, 1}, {1, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1} },
  {/* B_8 0b10010101 */  {1, 1}, {0, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1} },
  {/* B_9 0b10010110 */  {1, 1}, {0, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1} },
  {/* B_A 0b10011001 */  {1, 1}, {0, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1} },
  {/* B_B 0b10011010 */  {1, 1}, {0, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1} },
  {/* B_C 0b10100101 */  {1, 1}, {0, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1} },
  {/* B_D 0b10100110 */  {1, 1}, {0, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1}, {1, 1}, {0, 1} },
  {/* B_E 0b10101001 */  {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {0, 1}, {1, 1} },
  {/* B_F 0b10101010 */  {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1}, {1, 1}, {0, 1} },
};


void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Build " __FILE__ "/" __DATE__ "/" __TIME__);

  pinMode(pin, OUTPUT);
}

uint8_t
avr_crc8_ccitt(uint8_t * data, size_t len) {
  uint8_t crc = 0;
  while (len--) {
    crc = crc ^ *data++;
    for (int i = 0; i < 8; i++) {
      if (crc & 0x80) {
        crc <<= 1;
        crc ^= 0x07;
      } else {
        crc <<= 1;
      };
    }
  }
  return crc;
}

void loop() {
  static unsigned long last = 0;
  if (millis() - last > 1000) {
    last = millis();

    // fake 10 nodes, going on and off as they like it.
    //
    static unsigned int ii = 0;
    unsigned short id; 
    unsigned char  st = ii & 0x800 ? 0 : 255 ; // random(2) == 1 ? 0 : 255;
    id = 0x8000 + ii & 0x7FFF;
    ii++;
 
    unsigned char payload[ 4 ] = { (unsigned char)(( id >> 8) & 0xFF), (unsigned char)(id & 0xFF), st, 0 };

    payload[3] = avr_crc8_ccitt(payload, sizeof(payload) - 1);

    rec_t playlist[ 2 + 4 * (2 * 8) ], * p = playlist;

    *p++ = preamble[0];
    *p++ = preamble[1];
    for (size_t i = 0; i < sizeof(payload); i++) {
      for (size_t j = 0; j < 8; j++)
        *p++ = nibble2playlist[ (payload[i] >> 4) & 0xF ][j];
      for (size_t j = 0; j < 8; j++)
        *p++ = nibble2playlist[ payload[i] & 0xF  ][j];
    };

    for (size_t i = 0; i < 2 + 4 * (2 * 8) ; i++) {
      digitalWrite(pin, playlist[i].level);
      delayMicroseconds(playlist[i].duration * halfbit);
    };
    digitalWrite(pin, 0); // always end with the pin LOW

    Serial.printf("Sent: %04x is %s with CRC %02x (Datagram %02x.%02x.%02x.%02x)\n", id, st ? "on " : "off", payload[3], payload[0], payload[1], payload[2], payload[3]);
  }
}
