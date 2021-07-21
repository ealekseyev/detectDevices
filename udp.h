#pragma once

#include "names.h"
#include "ethernet.h"

struct __attribute__((__packed__)) udp_header {
    //uint8 zeroes;
    //uint8 protocol;
    //uint16 udp_len;
    uint16 src_port;
    uint16 dst_port;
    uint16 pack_len;
    uint16 checksum;
    // then comes the payload.
}; typedef struct udp_header UDPHeader;