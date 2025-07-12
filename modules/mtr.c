#include "mtr.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#define MAX_HOPS 30
#define TIMEOUT_SEC 1
#define DEFAULT_INTERVAL_MS 1000 // 1 second
#define DEFAULT_COUNT 10
#define PACKET_SIZE 64

// Structure to hold hop statistics
typedef struct {
    char ip_addr[INET_ADDRSTRLEN];
    int packets_sent;
    int packets_received;
    long long total_rtt;
    long long min_rtt;
    long long max_rtt;
} hop_stats;

int mtr_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: mtr <host> [port] [--interval <ms>] [--count <num>]\n");
        return 1;
    }

    const char *dest_host = argv[1];
    int port = 80; // Default to port 80 (though not directly used for raw ICMP)
    long long interval_ms = DEFAULT_INTERVAL_MS;
    int count = DEFAULT_COUNT;

    // Parse arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--interval") == 0 && (i + 1) < argc) {
            interval_ms = atoll(argv[++i]);
        } else if (strcmp(argv[i], "--count") == 0 && (i + 1) < argc) {
            count = atoi(argv[++i]);
        } else {
            // If it's not an option, assume it's a port, but for raw ICMP it's ignored
            // We keep it for consistency with TCP-based tools
            port = atoi(argv[i]);
        }
    }

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

    printf("MTR to %s (%s), %d hops max, %lldms interval, %d probes\n",
           dest_host, inet_ntoa(dest_addr.sin_addr), MAX_HOPS, interval_ms, count);

    hop_stats stats[MAX_HOPS];
    for (int i = 0; i < MAX_HOPS; i++) {
        memset(&stats[i], 0, sizeof(hop_stats));
        stats[i].min_rtt = -1; // Initialize min_rtt to a value that will always be overwritten
        stats[i].max_rtt = 0; // Initialize max_rtt
    }

    char send_packet[PACKET_SIZE];
    char recv_buffer[PACKET_SIZE * 2]; // Buffer for IP header + ICMP

    for (int probe_num = 0; probe_num < count; probe_num++) {
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
                // Treat as packet loss for this hop
                stats[ttl-1].packets_sent++;
                continue;
            }

            stats[ttl-1].packets_sent++;

            int bytes_recv = recvfrom(recv_sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);

            if (bytes_recv < 0) {
                // Timeout or error, treat as packet loss
            } else {
                struct iphdr *ip_hdr = (struct iphdr *)recv_buffer;
                struct icmphdr *recv_icmp_hdr = (struct icmphdr *)(recv_buffer + (ip_hdr->ihl * 4));

                char ip_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(recv_addr.sin_addr), ip_str, sizeof(ip_str));

                long long end_time = get_time_ms();
                long long rtt = end_time - start_time;

                // Update stats for this hop
                stats[ttl-1].packets_received++;
                stats[ttl-1].total_rtt += rtt;
                if (stats[ttl-1].min_rtt == -1 || rtt < stats[ttl-1].min_rtt) stats[ttl-1].min_rtt = rtt;
                if (rtt > stats[ttl-1].max_rtt) stats[ttl-1].max_rtt = rtt;

                // Copy IP to stats if not already set (or if it's the destination)
                if (strlen(stats[ttl-1].ip_addr) == 0 || recv_icmp_hdr->type == ICMP_ECHOREPLY) {
                    strncpy(stats[ttl-1].ip_addr, ip_str, INET_ADDRSTRLEN - 1);
                    stats[ttl-1].ip_addr[INET_ADDRSTRLEN - 1] = '\0';
                }

                if (recv_icmp_hdr->type == ICMP_ECHOREPLY) {
                    // Destination reached, no need to send more probes for higher TTLs
                    // Break out of inner loop, but continue outer loop for more probes
                    break;
                }
            }
        }
        // Wait for the next probe
        if (probe_num < count - 1) {
            struct timespec ts;
            ts.tv_sec = interval_ms / 1000;
            ts.tv_nsec = (interval_ms % 1000) * 1000000;
            nanosleep(&ts, NULL);
        }
    }

    // Print results
    printf("\n%-4s %-15s %-10s %-10s %-7s %-7s %-7s %-7s\n", "Hop", "IP Address", "Sent", "Recv", "Loss%", "Min", "Avg", "Max");
    printf("-------------------------------------------------------------------------------------\n");
    for (int i = 0; i < MAX_HOPS; i++) {
        if (stats[i].packets_sent > 0) {
            double loss_percent = (double)(stats[i].packets_sent - stats[i].packets_received) / stats[i].packets_sent * 100.0;
            long long avg_rtt = (stats[i].packets_received > 0) ? stats[i].total_rtt / stats[i].packets_received : 0;
            printf("%-4d %-15s %-10d %-10d %-6.2f%% %-7lld %-7lld %-7lld\n",
                   i + 1,
                   (strlen(stats[i].ip_addr) > 0) ? stats[i].ip_addr : "???",
                   stats[i].packets_sent,
                   stats[i].packets_received,
                   loss_percent,
                   stats[i].min_rtt == -1 ? 0 : stats[i].min_rtt,
                   avg_rtt,
                   stats[i].max_rtt);
        }
    }

    netan_close(send_sockfd);
    netan_close(recv_sockfd);
    return 0;
}