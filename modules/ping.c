#include "ping.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <netinet/ip_icmp.h>

#define PACKET_SIZE 64
#define PING_COUNT 4 // Default number of pings
#define PING_TIMEOUT_SEC 1 // Timeout for each ping

// Function to assess latency
const char* assess_latency(long long rtt) {
    if (rtt < 20) return "Good";
    if (rtt < 50) return "Normal";
    if (rtt < 100) return "Bad";
    return "Critical";
}

// Function to assess packet loss
const char* assess_packet_loss(double loss_percent) {
    if (loss_percent == 0) return "Good";
    if (loss_percent < 1) return "Normal";
    if (loss_percent < 5) return "Bad";
    return "Critical";
}

// Function to assess jitter
const char* assess_jitter(long long jitter) {
    if (jitter < 5) return "Good";
    if (jitter < 15) return "Normal";
    if (jitter < 30) return "Bad";
    return "Critical";
}

int ping_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ping <host>\n");
        return 1;
    }

    const char *dest_host = argv[1];
    struct hostent *host;
    struct sockaddr_in dest_addr, recv_addr;
    socklen_t recv_addr_len = sizeof(recv_addr);

    if ((host = gethostbyname(dest_host)) == NULL) {
        fprintf(stderr, "Cannot resolve host: %s\n", dest_host);
        return 1;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = 0; // Not used for raw sockets
    dest_addr.sin_addr = *((struct in_addr *)host->h_addr);

    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0) {
        perror("socket (requires root/admin privileges)");
        return 1;
    }

    // Set receive timeout
    struct timeval tv_recv = { .tv_sec = PING_TIMEOUT_SEC, .tv_usec = 0 };
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_recv, sizeof tv_recv);

    printf("Pinging %s (%s) with %d bytes of data:\n", dest_host, inet_ntoa(dest_addr.sin_addr), PACKET_SIZE);

    char send_packet[PACKET_SIZE];
    char recv_buffer[PACKET_SIZE * 2]; // Buffer for IP header + ICMP

    int packets_sent = 0;
    int packets_received = 0;
    long long total_rtt = 0;
    long long min_rtt = -1;
    long long max_rtt = 0;
    long long prev_rtt = -1;
    long long total_jitter = 0;
    int jitter_samples = 0;

    for (int i = 0; i < PING_COUNT; i++) {
        // Construct ICMP ECHO request
        struct icmphdr *icmp_hdr = (struct icmphdr *)send_packet;
        icmp_hdr->type = ICMP_ECHO;
        icmp_hdr->code = 0;
        icmp_hdr->checksum = 0;
        icmp_hdr->un.echo.id = getpid();
        icmp_hdr->un.echo.sequence = i;
        icmp_hdr->checksum = in_cksum((unsigned short *)send_packet, PACKET_SIZE);

        long long start_time = get_time_ms();

        // Send packet
        if (sendto(sockfd, send_packet, PACKET_SIZE, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) <= 0) {
            perror("sendto");
            netan_close(sockfd);
            return 1;
        }
        packets_sent++;

        int bytes_recv = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);

        if (bytes_recv < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("Request timed out.\n");
            } else {
                perror("recvfrom");
            }
        } else {
            struct iphdr *ip_hdr = (struct iphdr *)recv_buffer;
            struct icmphdr *recv_icmp_hdr = (struct icmphdr *)(recv_buffer + (ip_hdr->ihl * 4));

            if (recv_icmp_hdr->type == ICMP_ECHOREPLY && recv_icmp_hdr->un.echo.id == getpid()) {
                long long end_time = get_time_ms();
                long long rtt = end_time - start_time;

                packets_received++;
                total_rtt += rtt;
                if (min_rtt == -1 || rtt < min_rtt) min_rtt = rtt;
                if (rtt > max_rtt) max_rtt = rtt;

                if (prev_rtt != -1) {
                    total_jitter += abs(rtt - prev_rtt);
                    jitter_samples++;
                }
                prev_rtt = rtt;

                printf("Reply from %s: bytes=%d time=%lldms TTL=%d\n",
                       inet_ntoa(recv_addr.sin_addr), bytes_recv, rtt, ip_hdr->ttl);
            }
        }
    }

    printf("\n--- %s ping statistics ---\n", dest_host);
    printf("%d packets transmitted, %d received, %.2f%% packet loss\n",
           packets_sent, packets_received, (double)(packets_sent - packets_received) * 100.0 / packets_sent);

    if (packets_received > 0) {
        long long avg_rtt = total_rtt / packets_received;
        long long avg_jitter = (jitter_samples > 0) ? total_jitter / jitter_samples : 0;

        printf("Round-trip min/avg/max = %lld/%lld/%lld ms\n", min_rtt, avg_rtt, max_rtt);
        printf("Jitter (avg) = %lld ms\n", avg_jitter);

        printf("\nAssessment:\n");
        printf("  Latency: %lldms (%s)\n", avg_rtt, assess_latency(avg_rtt));
        printf("  Packet Loss: %.2f%% (%s)\n", (double)(packets_sent - packets_received) * 100.0 / packets_sent, assess_packet_loss((double)(packets_sent - packets_received) * 100.0 / packets_sent));
        printf("  Jitter: %lldms (%s)\n", avg_jitter, assess_jitter(avg_jitter));
    }

    netan_close(sockfd);
    return 0;
}