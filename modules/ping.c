#include "ping.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

// Function to get current time in milliseconds
long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int ping_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: ping <host> [port]\n");
        return 1;
    }

    const char *dest_host = argv[1];
    const int port = (argc > 2) ? atoi(argv[2]) : 80; // Default to port 80

    struct hostent *host;
    struct sockaddr_in dest_addr;

    if ((host = gethostbyname(dest_host)) == NULL) {
        fprintf(stderr, "Cannot resolve host: %s\n", dest_host);
        return 1;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr = *((struct in_addr *)host->h_addr);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Set socket to non-blocking
#ifdef _WIN32
    u_long mode = 1; // 1 to enable non-blocking mode
    ioctlsocket(sockfd, FIONBIO, &mode);
#else
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

    long long start_time = get_time_ms();

    int res = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (res < 0 && errno != EINPROGRESS) {
        perror("connect");
        netan_close(sockfd);
        return 1;
    }

    if (res == 0) { // Connected immediately
        long long end_time = get_time_ms();
        printf("Connected to %s:%d in %lld ms\n", dest_host, port, end_time - start_time);
        netan_close(sockfd);
        return 0;
    }

    // Use select to wait for connection
    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    tv.tv_sec = 2; // 2-second timeout
    tv.tv_usec = 0;

    res = select(sockfd + 1, NULL, &fdset, NULL, &tv);

    if (res < 0 && errno != EINTR) {
        perror("select");
        netan_close(sockfd);
        return 1;
    } else if (res > 0) {
        int so_error;
        socklen_t len = sizeof(so_error);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

        if (so_error == 0) {
            long long end_time = get_time_ms();
            printf("Connected to %s:%d in %lld ms\n", dest_host, port, end_time - start_time);
        } else {
            fprintf(stderr, "Connection failed: %s\n", strerror(so_error));
        }
    } else {
        fprintf(stderr, "Connection timed out\n");
    }

    netan_close(sockfd);
    return 0;
}