#ifndef _H_MAINSNODE
#define _H_MAINSNODE

#define MAINSNODE_MAXAGE (15 * 60) // Expect a report at least every 15 minutes.

// We force our enum into a byte - as it is also used
// at protocol level.
//
typedef enum : byte {
  MAINSNODE_STATE_OFF = 0x00,
  MAINSNODE_STATE_DEAD = 0x01, // not really sent by nodes - but state once we do not hear for a long time
  MAINSNODE_STATE_ON = 0xFF,
} messages_t;

// On the wire packet structure of the v2.00 packets.
//
typedef struct __attribute__((__packed__)) mainsnode_datagram {
  // raw - i.e. as coming in on the wire.
  union {
    uint8_t raw8[4];
    uint32_t raw32;
    struct {
      uint8_t payload[3];
      uint8_t crc; // _crc8_ccitt_update
    } raw;

    // unpacked - as how it should be decoded.
    struct {
      union {
        struct {
          uint8_t id8_high; // high is sent first; then the low.
          uint8_t id8_low;
        };
        uint16_t id16; // in network order (big endian).
      };
      messages_t state;
      uint8_t crc;
    };
  };
} mainsnode_datagram_t;

#endif
