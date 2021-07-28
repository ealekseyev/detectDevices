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
#include <vector>
#include <algorithm>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef uint32_t uint32;
typedef unsigned long long int uint64;

using std::vector;

bool isInArray(uint8* haystack, uint8* needle, uint8 searchlen, uint8 needlelen) {
    for(int i = 0; i < searchlen; i++) {
        if(haystack[i] == needle[0]) {
            // we already compared ind. 0
            bool match = true;
            for(int j = 1; j < needlelen; j++) {
                if(haystack[i+j] != needle[j]) {
                    match = false;
                    break;
                }
            } 
            if(match) {
                return true;
            }
        } else {
            continue;
        }
    }
    return false;
}

template <typename T>
bool contains(vector<T> vec, T item) {
    if (std::find(vec.begin(), vec.end(), item) != vec.end())
        return true;
    return false;
}

namespace names {
    char* pathToNotifier = "/home/admin/Documents/Code/detectDevices/sendNotif.py";
};