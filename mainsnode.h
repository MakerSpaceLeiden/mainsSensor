// Salient part of the current datagram.
//
typedef union {
		struct /* _attribute__((__packed__)) */ {
			#define MAINSNODE_HDR (0xA5)
			unsigned char hdr;	// fixed to A5 for this version.
			unsigned short node_id;	// network order.
			#define MAINSNODE_STATE_ON (0xFF)
			#define MAINSNODE_STATE_OFF (0)
			unsigned char state;	// 00 = off, 255 = On.
		} __attribute__((packed));
		unsigned char buff[4];
		uint32_t val;
	} mainsnode_datagram_t;

#define mainsnode_datagram_valid_hdr(x) ((x)[0] == MAINSNODE_HDR)
