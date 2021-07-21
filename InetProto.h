#pragma once

#include "udp.h"

struct __attribute__((__packed__)) ipv4_header {
    int version: 4;     // 0100 for ipv4 0110 for v6
    int hlen: 4;        // header length
    uint8 dscp;         // differentiated services field
    uint16 plen;         // total packet length
    uint16 pid;         // packet id
    int flags: 4;       // flags
    int foffset: 12;     // fragment offset
    uint8 ttl;          // time to live
    uint8 protocol;     // udp = 0x11
    uint16 checksum;    // checksum for entire header
    uint8 src_ip[4];    // source ip
    uint8 dst_ip[4];    // target ip
    // sometimes the src and dst ip from udp headers are put into here
}; typedef struct ipv4_header IPHeader;

struct __attribute__((__packed__)) ipv6_header {
    //TODO: fill in
}; typedef struct ipv6_header IPV6Header;

namespace inetProto {
    void getIPFromIface(char* ip, char* iface) {
        struct sockaddr_in addr;
        struct ifaddrs* ifaddr;
        struct ifaddrs* ifa;
        socklen_t addr_len;

        addr_len = sizeof (addr);
        getifaddrs(&ifaddr);

        // look which interface contains the wanted IP.
        // When found, ifa->ifa_name contains the name of the interface (eth0, eth1, ppp0...)
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (AF_INET == ifa->ifa_addr->sa_family) {
                // do not do one if with an &&. could cause memory
                // overflow error if they're not the same length

                if(strlen(ifa->ifa_name) == strlen(iface)) {
                    if (memcmp(ifa->ifa_name, iface, strlen(iface)) == 0) {
                        char* sockIP = inet_ntoa(((struct sockaddr_in *) ifa->ifa_addr)->sin_addr); // socket ip
                        strcpy(ip, sockIP);
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }

    void getIPFromIface(uint8* ip, char* iface) {
        char ip_str[16];
        getIPFromIface(ip_str, iface);
        uint8 ip_int[4]; // TODO: why does sscanf copy more than four bytes
        sscanf(ip_str, "%u.%u.%u.%u", ip_int, ip_int+1, ip_int+2, ip_int+3);
        memcpy(ip, ip_int, 4);
    }

#define ETH 0
#define ARP 1
#define ICMP 2
#define IGMP 3
#define UDP 4
#define TCP 5
#define DNS 6
#define MDNS 7
#define DHCP 8
#define SSL 9
#define IPV4 10
#define IPV6 11
#define NTP 12

    int getPackType(uint8* buffer, size_t len) {
        EthHeader ethHeader;
        memcpy((uint8 * ) & ethHeader, buffer, 14);
        if (ethHeader.ethertype == htons(0x0806)) {
            return ARP;
        } else if(ethHeader.ethertype == htons(0x86dd)) {
            return IPV6;
        } else if (ethHeader.ethertype == htons(0x0800)) {
            IPHeader ipHeader;
            memcpy((uint8 * ) & ipHeader, buffer + 14, sizeof(IPHeader));
            if (ipHeader.protocol == 1) {
                return ICMP;
            } else if (ipHeader.protocol == 2) {
                return IGMP;
            } else if (ipHeader.protocol == 6) {
                return TCP;
            } else if (ipHeader.protocol == 17) {
                UDPHeader udpHeader;
                memcpy((uint8 * ) & udpHeader, buffer + 14 + sizeof(IPHeader), sizeof(UDPHeader));
                if (udpHeader.dst_port == htons(53)) {
                    return DNS;
                } else if (udpHeader.dst_port == htons(5353)) {
                    return MDNS;
                } else if (udpHeader.dst_port == htons(67)) {
                    return DHCP;
                } else if (udpHeader.dst_port == htons(123)) {
                    return NTP;
                } else {
                    return UDP;
                }
            } else if (ipHeader.protocol == 4) {
                return IPV4;
            }
        } else {
            return ETH;
        }
    }
}