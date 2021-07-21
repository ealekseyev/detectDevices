#pragma once

#include <stdio.h>
#include "names.h"
#include "ethernet.h"
#include "dhcp.h"
#include "InetProto.h"
#include <ctime>
#include <chrono>

struct __attribute__((__packed__)) _dhcp_packet_full {
    EthHeader eth;
    IPHeader ip;
    UDPHeader udp;
    DHCPPayload dhcp;
}; typedef struct _dhcp_packet_full DHCPPacket;

int main(int argc, char** argv) {
    int sockfd = 0;
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
        printf("Error creating socket\n");
        return 1;
    }

    /// bind to ens9
    strcpy(ethernet::global_iface, argv[1]);
    ethernet::bindToIface(sockfd, argv[1]);
    ethernet::printIfaceDetails();

    uint8 lastConnDevice[6];

    std::time_t lastConnTime = 0;
    while(true) {
        int packType = -1, buflen = -1;
        while(packType != DHCP) {
            buflen = ethernet::listenForExclusiveBytes(sockfd, ethernet::private_buffer, ethernet::broadcast_mac);
            packType = inetProto::getPackType(ethernet::private_buffer, buflen);
            printf("Received packet: %d\n", packType);
        }
        DHCPPacket dhcpData;
        memcpy((uint8*) &dhcpData, ethernet::private_buffer, buflen < sizeof(DHCPPacket) ? buflen:sizeof(DHCPPacket));
        if(memcmp(lastConnDevice, dhcpData.eth.src_addr, 6) == 0 && 
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - lastConnTime < 30) {
            printf("\n%d seconds elapsed since last connection. Skipping\n\n", std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - lastConnTime);
	    lastConnTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // stacks up after a handful of packets
            continue;
        }
        
        ethernet::printPacket(ethernet::private_buffer, buflen);
        // send email abt new device on network
        char cmd[100] = "python3 ";
        strcat(cmd, names::pathToNotifier); strcat(cmd, " ");
        char mac[18]; sprintf(mac, "%0.2x:%0.2x:%0.2x:%0.2x:%0.2x:%0.2x", dhcpData.eth.src_addr[0], dhcpData.eth.src_addr[1], dhcpData.eth.src_addr[2],dhcpData.eth.src_addr[3], dhcpData.eth.src_addr[4], dhcpData.eth.src_addr[5]);
        strcat(cmd, mac); strcat(cmd, " CONN &");
        system(cmd);
        // make sure not to send the email twice
        memcpy(lastConnDevice, dhcpData.eth.src_addr, 6);
        lastConnTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    }
}
