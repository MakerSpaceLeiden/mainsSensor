#include <assert.h>

const int pin = 5;
const  int halfbit = 200; // microSeconds

typedef struct {
  int level;
  int duration;
} rec_t;

// normal manchester
#define B_0 /* 0b01010101 */  {0,1},{1,1},{0,1},{1,1},{0,1},{1,1},{0,1},{1,1}
#define B_1 /* 0b01010110 */  {0,1},{1,1},{0,1},{1,1},{0,1},{1,1},{1,1},{0,1}
#define B_2 /* 0b01011001 */  {0,1},{1,1},{0,1},{1,1},{1,1},{0,1},{0,1},{1,1}
#define B_3 /* 0b01011010 */  {0,1},{1,1},{0,1},{1,1},{1,1},{0,1},{1,1},{0,1}
#define B_4 /* 0b01100101 */  {0,1},{1,1},{1,1},{0,1},{0,1},{1,1},{0,1},{1,1}
#define B_5 /* 0b01100101 */  {0,1},{1,1},{1,1},{0,1},{0,1},{1,1},{0,1},{1,1}
#define B_6 /* 0b01101001 */  {0,1},{1,1},{1,1},{0,1},{1,1},{0,1},{0,1},{1,1}
#define B_7 /* 0b01101010 */  {0,1},{1,1},{1,1},{0,1},{1,1},{0,1},{1,1},{0,1}
#define B_8 /* 0b10010101 */  {1,1},{0,1},{0,1},{1,1},{0,1},{1,1},{0,1},{1,1}
#define B_9 /* 0b10010110 */  {1,1},{0,1},{0,1},{1,1},{0,1},{1,1},{1,1},{0,1}
#define B_A /* 0b10011001 */  {1,1},{0,1},{0,1},{1,1},{1,1},{0,1},{0,1},{1,1}
#define B_B /* 0b10011010 */  {1,1},{0,1},{0,1},{1,1},{1,1},{0,1},{1,1},{0,1}
#define B_C /* 0b10100101 */  {1,1},{0,1},{1,1},{0,1},{0,1},{1,1},{0,1},{1,1}
#define B_D /* 0b10100110 */  {1,1},{0,1},{1,1},{0,1},{0,1},{1,1},{1,1},{0,1}
#define B_E /* 0b10101001 */  {1,1},{0,1},{1,1},{0,1},{1,1},{0,1},{0,1},{1,1}
#define B_F /* 0b10101010 */  {1,1},{0,1},{1,1},{0,1},{1,1},{0,1},{1,1},{0,1}


rec_t playlist[] = {
  { 1, 4, },
  { 0, 4, }, // pre-amble

  // ID = 0x 8001
  //
  B_8, B_0, B_0, B_1,

  // On or OFF = 0x00
  B_0, B_0,

  // CRC8 (80010) = 0x1E
  B_1, B_E
};

int n = sizeof(playlist) / sizeof(rec_t);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Build " __FILE__ "/" __DATE__ "/" __TIME__);
  Serial.printf("Records: %d\n", n);

  assert(n == (2 + 8 * (4 + 2 + 2)));
  
  pinMode(pin, OUTPUT);
}

void loop() {
  for (int i = 0; i < n; i++) {
    digitalWrite(pin, playlist[i].level);
    delayMicroseconds(playlist[i].duration * halfbit);
  };
  digitalWrite(pin,0); // always end with the pin LOW
  
  delay(1000);
  Serial.println("tock");
}

