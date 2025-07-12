#include "scanner.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

int scanner_main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: scan <host> <start_port> [end_port]\n");
        return 1;
    }

    const char *dest_host = argv[1];
    int start_port = atoi(argv[2]);
    int end_port = (argc > 3) ? atoi(argv[3]) : start_port;

    struct hostent *host;
    struct sockaddr_in dest_addr;

    if ((host = gethostbyname(dest_host)) == NULL) {
        fprintf(stderr, "Cannot resolve host: %s\n", dest_host);
        return 1;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr = *((struct in_addr *)host->h_addr);

    printf("Scanning %s (%s) from port %d to %d\n", dest_host, inet_ntoa(dest_addr.sin_addr), start_port, end_port);

    for (int port = start_port; port <= end_port; port++) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            return 1;
        }

        dest_addr.sin_port = htons(port);

        // Set non-blocking
#ifdef _WIN32
        u_long mode = 1; // 1 to enable non-blocking mode
        ioctlsocket(sockfd, FIONBIO, &mode);
#else
        fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif

        connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

        fd_set fdset;
        struct timeval tv;
        FD_ZERO(&fdset);
        FD_SET(sockfd, &fdset);
        tv.tv_sec = 0;
        tv.tv_usec = 200000; // 200ms timeout

        if (select(sockfd + 1, NULL, &fdset, NULL, &tv) == 1) {
            int so_error;
            socklen_t len = sizeof so_error;
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error == 0) {
                printf("Port %d: Open\n", port);
            }
        }
        netan_close(sockfd);
    }

    return 0;
}
