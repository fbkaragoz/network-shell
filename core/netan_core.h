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

#endif // NETAN_CORE_H