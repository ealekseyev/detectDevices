#pragma once

#include <stdio.h>
#include "names.h"
#include "ethernet.h"
#include "dhcp.h"
#include "InetProto.h"
#include <ctime>
#include <chrono>
#include <vector>
#include <algorithm>
#include <iostream>

using std::vector;
using std::cout, std::endl;

struct __attribute__((__packed__)) _dhcp_packet_full {
    EthHeader eth;
    IPHeader ip;
    UDPHeader udp;
    DHCPPayload dhcp;
}; typedef struct _dhcp_packet_full DHCPPacket;
struct __attribute__((__packed__)) _mdns_packet_proto {
    EthHeader eth;
    IPHeader ip;
    UDPHeader udp;
    uint8 mdns[100];
}; typedef struct _mdns_packet_proto MDNSPacket;

vector<MACAddr> apple_devices;

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

    // stores last device and time dhcp was received
    MACAddr lastConnDevice;
    std::time_t lastConnTime = 0;
    while(true) {
        int packType = -1, buflen = -1;
        // scan for dhcp and mdns packets (LAN auth and airplay pings)
        while (packType != DHCP && packType != MDNS) {
            buflen = ethernet::readRawBytes(sockfd, ethernet::private_buffer);
            //buflen = ethernet::listenForExclusiveBytes(sockfd, ethernet::private_buffer, ethernet::broadcast_mac);
            packType = inetProto::getPackType(ethernet::private_buffer, buflen);
        }
        printf("Received packet: %d\n", packType);
        /// Apple devices constantly ping for new devices with keyword "airplay" in mdns
        if (packType == MDNS) {
            try {
                MDNSPacket mdns;
                memcpy((uint8 *) &mdns, ethernet::private_buffer,
                       sizeof(MDNSPacket) < buflen ? sizeof(MDNSPacket) : buflen);
                MACAddr src_mac;
                memcpy(&src_mac, &mdns.eth.src_addr, 6);
                // check if packet is an airplay ping
                bool is_apple = isInArray(mdns.mdns, (uint8 *) "airplay",
                                          buflen - sizeof(IPHeader) - sizeof(UDPHeader) - sizeof(EthHeader),
                                          strlen("airplay"));
                // if it is, update the apple devices array
                if (is_apple == true) {
                    printf("Airplay ping sniffed from "); /// apple packet
                    ethernet::printMAC(mdns.eth.src_addr.addr);
                    printf(" - ");
                    // if not present, append
                    /// from here bad_alloc
                    if (!contains(apple_devices, src_mac)) {
                        printf("appended\n");
                        apple_devices.push_back(src_mac);
                    } else {
                        printf("already in whitelist\n");
                    }
                    printf("Apple devices:\n");
                    for (int i = 0; i < apple_devices.size(); i++) {
                        printf("\t%0.2x:%0.2x:%0.2x:%0.2x:%0.2x:%0.2x\n",
                               apple_devices[i].addr[0], apple_devices[i].addr[1], apple_devices[i].addr[2],
                               apple_devices[i].addr[3], apple_devices[i].addr[4], apple_devices[i].addr[5]);
                    }
                }
            } catch (std::bad_alloc &ba) {
                cout << endl << "in mdns" << endl;
                return 1;
            }
        } else if (packType == DHCP) {
            try {
                DHCPPacket dhcpData;
                memcpy((uint8 *) &dhcpData, ethernet::private_buffer,
                       buflen < sizeof(DHCPPacket) ? buflen : sizeof(DHCPPacket));
                MACAddr src_mac;
                memcpy(&src_mac, &dhcpData.eth.src_addr, 6);
                //TODO: could move this outside of DHCP/MDNS ifs ^^

                /// do not continue if it is from the same chain of dhcp packets
                if (lastConnDevice == dhcpData.eth.src_addr &&
                    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - lastConnTime < 60) {
                    printf("%d seconds elapsed since last connection. Skipping\n",
                           std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - lastConnTime);
                    continue;
                }

                ethernet::printPacket(ethernet::private_buffer, buflen);
                bool is_apple = contains(apple_devices, src_mac);

                /// send email abt new device on network
                char cmd[100] = "python3 ";
                strcat(cmd, names::pathToNotifier);
                strcat(cmd, " ");
                char mac[18];
                sprintf(mac, "%0.2x:%0.2x:%0.2x:%0.2x:%0.2x:%0.2x",
                        dhcpData.eth.src_addr.addr[0], dhcpData.eth.src_addr.addr[1], dhcpData.eth.src_addr.addr[2],
                        dhcpData.eth.src_addr.addr[3], dhcpData.eth.src_addr.addr[4], dhcpData.eth.src_addr.addr[5]);
                strcat(cmd, mac);
                strcat(cmd, " CONN ");
                if (is_apple)
                    strcat(cmd, "ISAPPL &");
                else
                    strcat(cmd, "NOAPPL &");
                printf("Executing cmd: %s\nSent!\n", cmd);
                system(cmd);
                // make sure not to send the email twice
                memcpy(&lastConnDevice, &dhcpData.eth.src_addr, 6);
                lastConnTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            } catch (std::bad_alloc &ba) {
                cout << endl << "in dhcp" << endl;
                return 1;
            }
        }
    }
}