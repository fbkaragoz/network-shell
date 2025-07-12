#include "traceroute.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define MAX_HOPS 30
#define TIMEOUT_SEC 1
#define PACKET_SIZE 64

int traceroute_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: trace <host>\n");
        return 1;
    }

    const char *dest_host = argv[1];
    struct hostent *host_info;
    struct sockaddr_in dest_addr, recv_addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    if ((host_info = gethostbyname(dest_host)) == NULL) {
        fprintf(stderr, "Cannot resolve host: %s\n", dest_host);
        return 1;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = 0; // Not used for raw sockets
    dest_addr.sin_addr = *((struct in_addr *)host_info->h_addr);

    int send_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    int recv_sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (send_sockfd < 0 || recv_sockfd < 0) {
        perror("socket (requires root/admin privileges)");
        return 1;
    }

    // Set receive timeout
    struct timeval tv_recv = { .tv_sec = TIMEOUT_SEC, .tv_usec = 0 };
    setsockopt(recv_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_recv, sizeof tv_recv);

    printf("Traceroute to %s (%s), %d hops max\n", dest_host, inet_ntoa(dest_addr.sin_addr), MAX_HOPS);

    char send_packet[PACKET_SIZE];
    char recv_buffer[PACKET_SIZE * 2]; // Buffer for IP header + ICMP

    for (int ttl = 1; ttl <= MAX_HOPS; ttl++) {
        // Set TTL for outgoing packet
        if (setsockopt(send_sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
            perror("setsockopt TTL");
            netan_close(send_sockfd);
            netan_close(recv_sockfd);
            return 1;
        }

        // Construct ICMP ECHO request
        struct icmphdr *icmp_hdr = (struct icmphdr *)send_packet;
        icmp_hdr->type = ICMP_ECHO;
        icmp_hdr->code = 0;
        icmp_hdr->checksum = 0;
        icmp_hdr->un.echo.id = getpid();
        icmp_hdr->un.echo.sequence = ttl; // Use TTL as sequence for simplicity
        icmp_hdr->checksum = in_cksum((unsigned short *)send_packet, PACKET_SIZE);

        long long start_time = get_time_ms();

        // Send packet
        if (sendto(send_sockfd, send_packet, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) <= 0) {
            perror("sendto");
            netan_close(send_sockfd);
            netan_close(recv_sockfd);
            return 1;
        }

        printf("%2d: ", ttl);
        fflush(stdout);

        int bytes_recv = recvfrom(recv_sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);

        if (bytes_recv < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("* (timeout)\n");
            } else {
                perror("recvfrom");
            }
        } else {
            struct iphdr *ip_hdr = (struct iphdr *)recv_buffer;
            struct icmphdr *recv_icmp_hdr = (struct icmphdr *)(recv_buffer + (ip_hdr->ihl * 4));

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(recv_addr.sin_addr), ip_str, sizeof(ip_str));

            long long end_time = get_time_ms();
            long long rtt = end_time - start_time;

            // Try to resolve hostname
            char hostname[NI_MAXHOST];
            struct sockaddr_in temp_addr;
            memset(&temp_addr, 0, sizeof(temp_addr));
            temp_addr.sin_family = AF_INET;
            temp_addr.sin_addr = recv_addr.sin_addr;

            if (getnameinfo((struct sockaddr *)&temp_addr, sizeof(temp_addr), hostname, sizeof(hostname), NULL, 0, NI_NAMEREQD) == 0) {
                printf("%s (%s) %lld ms\n", hostname, ip_str, rtt);
            } else {
                printf("%s %lld ms\n", ip_str, rtt);
            }

            if (recv_icmp_hdr->type == ICMP_ECHOREPLY) {
                printf("Traceroute complete.\n");
                netan_close(send_sockfd);
                netan_close(recv_sockfd);
                return 0;
            }
        }
    }

    netan_close(send_sockfd);
    netan_close(recv_sockfd);
    return 0;
}