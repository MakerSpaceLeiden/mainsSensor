#ifndef _H_MAINSNODE
#define _H_MAINSNODE

// On the wire packet structure of the v2.00 packets.
//
typedef struct __attribute__((__packed__)) mainsnode_datagram {
  union {
    uint8_t raw8[4];
    uint32_t raw32;
    struct {
	uint8_t payload[3];
        uint8_t crc;
    } raw;
    struct {
      union {
        struct {
          uint8_t id8_high; // high is sent first; then the low.
          uint8_t id8_low;
        };
        uint16_t id16; // in network order
      };
      uint8_t state;
      uint8_t crc;
    };
  };
} mainsnode_datagram_t;

typedef enum {
  MAINSNODE_STATE_OFF = 0x00,
  MAINSNODE_STATE_ON = 0xFF,
} messages_t;
#endif
