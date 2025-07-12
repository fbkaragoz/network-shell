#include "traceroute.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#define MAX_HOPS 30
#define TIMEOUT_SEC 1

// Function to get current time in milliseconds
long long get_time_ms_traceroute() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int traceroute_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: trace <host> [port]\n");
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

    printf("TCP-based traceroute to %s (%s) on port %d, %d hops max\n",
           dest_host, inet_ntoa(dest_addr.sin_addr), port, MAX_HOPS);

    for (int ttl = 1; ttl <= MAX_HOPS; ttl++) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            return 1;
        }

        // Set TTL
        if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
            perror("setsockopt TTL");
            netan_close(sockfd);
            break;
        }

        // Set non-blocking
#ifdef _WIN32
        u_long mode = 1; // 1 to enable non-blocking mode
        ioctlsocket(sockfd, FIONBIO, &mode);
#else
        fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

        long long start_time = get_time_ms_traceroute();
        int res = connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        printf("%2d: ", ttl);
        fflush(stdout);

        if (res < 0 && errno != EINPROGRESS) {
            printf("* (connect error: %s)\n", strerror(errno));
            netan_close(sockfd);
            continue;
        }

        fd_set fdset;
        struct timeval tv;
        FD_ZERO(&fdset);
        FD_SET(sockfd, &fdset);
        tv.tv_sec = TIMEOUT_SEC;
        tv.tv_usec = 0;

        res = select(sockfd + 1, NULL, &fdset, NULL, &tv);

        if (res <= 0) { // Timeout or error
            printf("* (timeout)\n");
        } else {
            int so_error;
            socklen_t len = sizeof(so_error);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);

            long long end_time = get_time_ms_traceroute();
            long long rtt = end_time - start_time;

            if (so_error == 0 || so_error == ECONNREFUSED) {
                printf("%s:%d reached in %lld ms\n", inet_ntoa(dest_addr.sin_addr), port, rtt);
                netan_close(sockfd);
                return 0; // Destination reached
            } else {
                 // This is an intermediate hop. We can't get its IP without raw sockets,
                 // but we know we haven't timed out.
                 printf("* (%lld ms)\n", rtt);
            }
        }
        netan_close(sockfd);
    }

    return 0;
}