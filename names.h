#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <ifaddrs.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef uint32_t uint32;

size_t strlength(const char* str) {
    size_t c = 0;
    while(true) {
        if(*(str+c) == 0)
            return c;
        c++;
    }
}

namespace names {
    char* pathToNotifier = "/home/admin/Documents/Code/detectDevices/sendNotif.py";
};