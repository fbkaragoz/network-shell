#ifndef NETAN_CORE_H
#define NETAN_CORE_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/time.h>
    #include <netinet/ip.h>
    #include <netinet/ip_icmp.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to initialize networking (for Windows)
static inline int netan_init(void) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }
#endif
    return 0;
}

// Function to clean up networking (for Windows)
static inline void netan_cleanup(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Cross-platform close socket function
static inline int netan_close(int sockfd) {
#ifdef _WIN32
    return closesocket(sockfd);
#else
    return close(sockfd);
#endif
}

// Function to get current time in milliseconds
static inline long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// ICMP checksum function
static inline unsigned short in_cksum(unsigned short *addr, int len) {
    int sum = 0;
    unsigned short *w = addr;
    unsigned short answer = 0;

    while (len > 1) {
        sum += *w++;
        len -= 2;
    }

    if (len == 1) {
        *(unsigned char *)&answer = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}

#endif // NETAN_CORE_H
