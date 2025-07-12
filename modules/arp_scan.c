#include "arp_scan.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if_arp.h>

// ARP header structure
struct arp_header {
    unsigned short htype;    // Hardware type (1 for Ethernet)
    unsigned short ptype;    // Protocol type (0x0800 for IPv4)
    unsigned char hlen;      // Hardware address length (6 for Ethernet)
    unsigned char plen;      // Protocol address length (4 for IPv4)
    unsigned short oper;     // Operation (1 for request, 2 for reply)
    unsigned char sha[6];    // Sender hardware address
    unsigned char spa[4];    // Sender protocol address
    unsigned char tha[6];    // Target hardware address
    unsigned char tpa[4];    // Target protocol address
};

int arp_scan_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: arp_scan <interface_name>\n");
        return 1;
    }

    const char *if_name = argv[1];

    int sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0) {
        perror("socket (requires root/admin privileges)");
        return 1;
    }

    struct ifreq if_idx;
    memset(&if_idx, 0, sizeof(struct ifreq));
    strncpy(if_idx.ifr_name, if_name, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
        perror("SIOCGIFINDEX");
        netan_close(sockfd);
        return 1;
    }

    struct ifreq if_mac;
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, if_name, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
        perror("SIOCGIFHWADDR");
        netan_close(sockfd);
        return 1;
    }

    struct ifreq if_ip;
    memset(&if_ip, 0, sizeof(struct ifreq));
    strncpy(if_ip.ifr_name, if_name, IFNAMSIZ - 1);
    if (ioctl(sockfd, SIOCGIFADDR, &if_ip) < 0) {
        perror("SIOCGIFADDR");
        netan_close(sockfd);
        return 1;
    }

    struct sockaddr_ll socket_address;
    socket_address.sll_family = AF_PACKET;
    socket_address.sll_protocol = htons(ETH_P_ARP);
    socket_address.sll_ifindex = if_idx.ifr_ifindex;
    socket_address.sll_hatype = ARPHRD_ETHER;
    socket_address.sll_pkttype = PACKET_BROADCAST;
    socket_address.sll_halen = ETH_ALEN;
    socket_address.sll_addr[6] = 0x00;
    socket_address.sll_addr[7] = 0x00;

    unsigned char broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    unsigned char sender_mac[6];
    memcpy(sender_mac, if_mac.ifr_hwaddr.sa_data, 6);

    struct in_addr sender_ip = ((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr;
    unsigned int subnet_mask = ((struct sockaddr_in *)&if_ip.ifr_netmask)->sin_addr.s_addr;
    unsigned int network_address = sender_ip.s_addr & subnet_mask;

    printf("Scanning local network on interface %s (%s). This may take a moment...\n", if_name, inet_ntoa(sender_ip));

    // Loop through all possible IPs in the subnet
    for (unsigned int i = 1; i < 255; i++) { // Assuming /24 subnet for simplicity
        unsigned int target_ip_int = network_address | htonl(i);
        struct in_addr target_ip;
        target_ip.s_addr = target_ip_int;

        // Ethernet header
        unsigned char ethernet_frame[ETH_FRAME_LEN];
        memcpy(ethernet_frame, broadcast_mac, 6); // Destination MAC
        memcpy(ethernet_frame + 6, sender_mac, 6); // Source MAC
        ethernet_frame[12] = ETH_P_ARP / 256;
        ethernet_frame[13] = ETH_P_ARP % 256;

        // ARP header
        struct arp_header *arp_hdr = (struct arp_header *)(ethernet_frame + ETH_HLEN);
        arp_hdr->htype = htons(ARPHRD_ETHER);
        arp_hdr->ptype = htons(ETH_P_IP);
        arp_hdr->hlen = ETH_ALEN;
        arp_hdr->plen = 4;
        arp_hdr->oper = htons(ARPOP_REQUEST);
        memcpy(arp_hdr->sha, sender_mac, 6);
        memcpy(arp_hdr->spa, &sender_ip, 4);
        memcpy(arp_hdr->tha, broadcast_mac, 6); // Target MAC (unknown)
        memcpy(arp_hdr->tpa, &target_ip, 4);

        if (sendto(sockfd, ethernet_frame, ETH_FRAME_LEN, 0, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0) {
            perror("sendto");
        }
    }

    // Set a timeout for receiving ARP replies
    struct timeval tv_recv = { .tv_sec = 5, .tv_usec = 0 }; // 5 second timeout for replies
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_recv, sizeof tv_recv);

    printf("\nDiscovered Devices:\n");
    printf("---------------------\n");

    unsigned char recv_buffer[ETH_FRAME_LEN];
    while (recvfrom(sockfd, recv_buffer, ETH_FRAME_LEN, 0, NULL, NULL) > 0) {
        struct ethhdr *eth_hdr = (struct ethhdr *)recv_buffer;
        if (ntohs(eth_hdr->h_proto) == ETH_P_ARP) {
            struct arp_header *arp_hdr = (struct arp_header *)(recv_buffer + ETH_HLEN);
            if (ntohs(arp_hdr->oper) == ARPOP_REPLY) {
                char sender_ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, arp_hdr->spa, sender_ip_str, sizeof(sender_ip_str));
                printf("IP: %-15s MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                       sender_ip_str,
                       arp_hdr->sha[0], arp_hdr->sha[1], arp_hdr->sha[2],
                       arp_hdr->sha[3], arp_hdr->sha[4], arp_hdr->sha[5]);
            }
        }
    }

    netan_close(sockfd);
    return 0;
}
