#pragma once

#include "names.h"
#include <cstring>

/// all uint16+ variables must be converted from network to host byte order on read.

struct __attribute__((__packed__)) eth_header {
    uint8 dst_addr[6];
    uint8 src_addr[6];
    uint16 ethertype;
}; typedef struct eth_header EthHeader;

namespace ethernet {
    uint8 broadcast_mac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    uint8 private_buffer[1000]; // private buffer for all packets
    char global_iface[IFNAMSIZ];
    /// converts char_ip into a uint array @ int_ip
    inline void IP_ston(uint8* int_ip, char* char_ip) {
        sscanf(char_ip, "%d.%d.%d.%d", int_ip, int_ip+1, int_ip+2, int_ip+3);
    }
    /// stores MAC address of interface passed through 'name' into 'retval'
    // verified
    void getIfaceMAC(uint8* retval, char* name) {
        char iface_mac[30];
        //strcpy(iface_mac, "78:7b:8a:bf:df:e3");
        strcpy(iface_mac, "/sys/class/net/");
        strcat(iface_mac, name);
        strcat(iface_mac, "/address");

        FILE *fp = fopen(iface_mac, "r");
        //char mac_str[18];
        //fgets(mac_str, 18, fp);
        //memset(iface_mac, 0, 30) // probably redundant
        fgets(iface_mac, 18, fp);
        sscanf(iface_mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &retval[0], &retval[1], &retval[2], &retval[3], &retval[4], &retval[5]);
    }

    // gets index of interface
    int getIfaceIfindex(char* iface) {
        return if_nametoindex(iface);
        /*struct ifreq ifr;
        ifr.ifr_ifindex =
        if(ioctl(NULL, SIOCGIFINDEX, &ifr) == -1)
            return -1;
        return ifr.ifr_name*/
    }

    /// gets interface of associated index
    void getIfindexIface(char* ret, int ifindex) {
        if_indextoname(ifindex, ret);
    }

    /// combine getSocketIfindex and getIfindexIface
    //TODO: figure out why getSocketIfindex does not work in here
    bool getIfaceBoundToSocket(char* ret, int socket) {
        /*struct ifreq ifr;
        if(ioctl(socket, SIOCGIFINDEX, &ifr) == -1)
            return false;
        getIfindexIface(ret, ifr.ifr_ifindex);
        return true;*/
        strcpy(ret, global_iface);
        return true;
    }

    // TODO: v problematic
    inline int getSocketIfr_ifindex(int socket) {
        char iface[IFNAMSIZ];
        getIfaceBoundToSocket(iface, socket);
        printf(iface);
        int index = getIfaceIfindex(iface);
        printf("\nifindex: %d\n", index);
        return index;
    }

    /// returns interface associated with iface_ip (uint8[4] array) into ret
    void getIfaceFromIP(char* ret, char* iface_ip) {
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
                char* sockIP = inet_ntoa(((struct sockaddr_in *) ifa->ifa_addr)->sin_addr); // socket ip
                // do not do one if with an &&. could cause memory
                // overflow error if they're not the same length
                if(strlen(sockIP) == strlen(iface_ip)) {
                    if (memcmp(sockIP, iface_ip, strlen(sockIP)) == 0) {
                        strcpy(ret, ifa->ifa_name); // return iface linked to ip
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
    }

    /// same as above function except with uint8[4] ip format
    void getIfaceFromIP(char* ret, uint8* iface_ip) {
        char iface_ip_char[16];
        sprintf(iface_ip_char, "%d.%d.%d.%d",
                iface_ip[0], iface_ip[1], iface_ip[2], iface_ip[3]);
        getIfaceFromIP(ret, iface_ip_char);
    }

    /// binds sockfd to interface iface
    bool bindToIface(int sockfd, char* iface) {
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr)); // set memory to all zeroes
        memcpy(ifr.ifr_name, iface, strlen(iface)); // denote which interface to use
        if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *) &ifr, sizeof(ifr)) < 0) {
            fprintf(stderr, "Error binding socket to device %s, try running with sudo\n", iface);
            return false;
        }
        return true;
    }

    /// writes an arp packet given EthHeader and ARPPacket structs;
    /// @deprecated old do not use
    void write_arp(int sock, void* ethHeader, int size_e, void* arpPack, int size_a, int sock_ifr_ifindex) {
        uint8 packet[64]; //TODO: check if this still works
        struct sockaddr_ll socket_address;
        //socket_address.sll_family = AF_PACKET;
        /* Index of the network device */
        socket_address.sll_ifindex = sock_ifr_ifindex;
        /* Address length*/
        socket_address.sll_halen = ETH_ALEN;
        /* Destination MAC */
        socket_address.sll_addr[0] = ((EthHeader*)ethHeader)->dst_addr[0];
        socket_address.sll_addr[1] = ((EthHeader*)ethHeader)->dst_addr[1];
        socket_address.sll_addr[2] = ((EthHeader*)ethHeader)->dst_addr[2];
        socket_address.sll_addr[3] = ((EthHeader*)ethHeader)->dst_addr[3];
        socket_address.sll_addr[4] = ((EthHeader*)ethHeader)->dst_addr[4];
        socket_address.sll_addr[5] = ((EthHeader*)ethHeader)->dst_addr[5];

        // convert structs to arrays and stitch them together
        memset(packet, 0, 64);
        memcpy(packet, (uint8*)ethHeader, size_e);
        memcpy((packet+size_e), (uint8*)arpPack, size_a);
        // print constructed packet
        for(int i = 0; i < 8; i++) {
            for(int j = 0; j < 8; j++) {
                printf("%0.2x ", packet[(i*8)+j]);
            }
            printf("\n");
        }
        //wireshark seems to like 60 more than 64
        int x = sendto(sock, (void*) packet, 60, 0, (const struct sockaddr*)&socket_address, sizeof(socket_address));
        if(x < 0) fprintf(stderr, "Err: write_arp\n");
        else printf("%d bytes written\n", x);
        //if(sendto(sock, (void*) packet, sizeof(packet), 0, (const struct sockaddr*)&socket_address, sizeof(socket_address)) < 0) {
        //   printf("an error occurred!");
        //}
    }

    /// writes byte array stored in packet
    /// @deprecated - use write_packet
    void write_packet_raw(int sock, void* packet, int packlen, struct sockaddr_ll addr) {
        #ifdef _PRINT
        // print constructed packet
        for(int i = 0; i < 12; i++) {
            for(int j = 0; j < 12; j++) {
                printf("%0.2x ", packet[(i*8)+j]);
            }
            printf("\n");
        }
        #endif
        int x = sendto(sock, (void*) packet, packlen, 0, (const struct sockaddr*)&addr, sizeof(addr));
        if(x < 0) {
            fprintf(stderr, "Err: write_packet_raw: %d\n", x);
        }
    }
    /// writes byte array stored in packet
    bool write_packet(int sock, void* packet, int packlen) {
#ifdef _PRINT
        // print constructed packet
        for(int i = 0; i < 12; i++) {
            for(int j = 0; j < 12; j++) {
                printf("%0.2x ", packet[(i*8)+j]);
            }
            printf("\n");
        }
#endif
        struct sockaddr_ll sock_addr;
        /* Index of the network device */
        sock_addr.sll_ifindex = getSocketIfr_ifindex(sock); // TODO: make this work
        /* Address length*/
        sock_addr.sll_halen = ETH_ALEN;
        /* Destination MAC */
        sock_addr.sll_addr[0] = ((uint8*) packet)[0]; // first six eth bytes always dst MAC
        sock_addr.sll_addr[1] = ((uint8*) packet)[1];
        sock_addr.sll_addr[2] = ((uint8*) packet)[2];
        sock_addr.sll_addr[3] = ((uint8*) packet)[3];
        sock_addr.sll_addr[4] = ((uint8*) packet)[4];
        sock_addr.sll_addr[5] = ((uint8*) packet)[5];
        int x = sendto(sock, (void*) packet, packlen, 0, (const struct sockaddr*)&sock_addr, sizeof(sock_addr));
        if(x < 0) {
            fprintf(stderr, "Err: write_packet: %d\n", x);
            return false;
        }
        return true;
    }

    /// reads and returns next packet; returns buffer length
    // TODO: could be faulty if packet exceeds buffer boundaries; this applies to dependent functions too
    int readRawBytes(int sock, uint8* buffer) {
        while(true) {
            int len = recvfrom(sock, buffer, 65536, 0, nullptr, nullptr); // don't care about any other information but the packet
            const uint8 blacklist[6] = {0,0,0,0,0,0};
            if (memcmp(blacklist, buffer, 6) == 0) {
                continue;
            } else {
                return len;
            }
        }
    }

    /// reads and returns next packet addressed to broadcast or specified MAC
    int listenForAddressedBytes(int sock, uint8* buffer, uint8* mac) {
        while(true) {
            int bytes = readRawBytes(sock, buffer);
            // if it's a broadcast or to specified MAC
            if(memcmp(buffer, mac, 6) == 0 || 
            (buffer[0] == buffer[1] == buffer[2] == buffer[3] == buffer[4] == buffer[5] == 0xFF)) {
                return bytes;
            }
        }
    }

    /// reads and returns next packet addressed to exclusively specified MAC
    int listenForExclusiveBytes(int sock, uint8* buffer, uint8* mac) {
        while(true) {
            int bytes = readRawBytes(sock, buffer);
            // if it's addressed to specified MAC
            if(memcmp(buffer, mac, 6) == 0) { // remember memcmp returns 0 if they are equivalent
                return bytes;
            }
        }
    }

    /// reads and returns next packet addressed to exclusively specified MAC from specified src
    int listenForExclusiveBytesFrom(int sock, uint8* buffer, uint8* mac, uint8* src) {
        while(true) {
            int bytes = readRawBytes(sock, buffer);
            // if it's addressed to specified MAC
            if(memcmp(buffer, mac, 6) == 0 && memcmp(buffer+6, src, 6) == 0) { // remember memcmp returns 0 if they are equivalent
                return bytes;
            }
        }
    }
    // TODO: check if it works. idk if little endian is required. Swap indeces 12 and 13 if it is
    /// reads and returns next packet addressed to exclusively specified MAC and specified protocol
    int listenForDetailedPacket(int sock, uint8* buffer, EthHeader* details) {
        while(true) {
            int bytes = readRawBytes(sock, buffer);
            // if it's addressed to specified MAC
            if(memcmp((uint8*) details, buffer, 14) == 0) {
                return bytes;
            }
            /*if(memcmp(buffer, mac, 6) == 0 && (buffer[12] == protocol >> 8) && buffer[13] == (protocol & 0x00ff)) { // remember memcmp returns 0 if they are equivalent
                return bytes;
            }*/
        }
    }

    void printPacket(uint8* buffer, size_t buflen, int width) {
        // print constructed packet
        //const int width = 16;
        for(int i = 0; i < ((buflen%width == 0) ? buflen/width:((buflen/width)+1)); i++) { // how many rows
            for(int j = 0; j < width; j++) { // how many columns in a row
                if((i*width)+j > buflen) {
                    printf("-- ");
                } else {
                    printf("%0.2x ", buffer[(i * width) + j]);
                }
            }
            printf("\n");
        }
    }

    void printPacket(uint8* buffer, size_t buflen) {
        printPacket(buffer, buflen, 12); // default 12 bytes per line
    }

    void printMAC(uint8* mac) {
        printf("%0.2x:%0.2x:%0.2x:%0.2x:%0.2x:%0.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
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

    void printIfaceDetails() {
        printf("Iface: %s\n", global_iface);
        uint8 mac[6];
        getIfaceMAC(mac, global_iface);
        printf("MAC: %0.2x:%0.2x:%0.2x:%0.2x:%0.2x:%0.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        getIPFromIface(mac, global_iface);
        printf("IP: %d.%d.%d.%d\n", mac[0], mac[1], mac[2], mac[3]);
    }
};

class EthWrapper {
private:
    int sock;
public:
    EthHeader ethHeader;
    EthWrapper(int sock) {
        this->sock = sock;
    }

    // return bytes written
    int write(uint8* buffer, size_t buflen) {
        return (int)ethernet::write_packet(this->sock, buffer, buflen);
    }

    int read(uint8* buffer) {
        EthHeader recvHeader;
        memcpy(recvHeader.dst_addr, ethHeader.src_addr, 6);
        memcpy(recvHeader.src_addr, ethHeader.dst_addr, 6);
        recvHeader.ethertype = ethHeader.ethertype;
        return ethernet::listenForDetailedPacket(this->sock, buffer, &recvHeader);
    }
};