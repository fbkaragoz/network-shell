#include "dns.h"
#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include <netdb.h>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

int dns_main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: dns <hostname_or_ip>\n");
        return 1;
    }

    const char *query = argv[1];
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];
    char hoststr[NI_MAXHOST];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    // Try to interpret as IP address first for reverse lookup
    struct sockaddr_storage ss;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)&ss;
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&ss;

    if (inet_pton(AF_INET, query, &(ipv4->sin_addr)) == 1) {
        // It's an IPv4 address, do a reverse lookup
        ipv4->sin_family = AF_INET;
        if ((status = getnameinfo((struct sockaddr *)ipv4, sizeof(struct sockaddr_in), hoststr, sizeof(hoststr), NULL, 0, 0)) == 0) {
            printf("Reverse DNS for %s: %s\n", query, hoststr);
        } else {
            fprintf(stderr, "Reverse DNS lookup failed for %s: %s\n", query, gai_strerror(status));
        }
    } else if (inet_pton(AF_INET6, query, &(ipv6->sin6_addr)) == 1) {
        // It's an IPv6 address, do a reverse lookup
        ipv6->sin6_family = AF_INET6;
        if ((status = getnameinfo((struct sockaddr *)ipv6, sizeof(struct sockaddr_in6), hoststr, sizeof(hoststr), NULL, 0, 0)) == 0) {
            printf("Reverse DNS for %s: %s\n", query, hoststr);
        } else {
            fprintf(stderr, "Reverse DNS lookup failed for %s: %s\n", query, gai_strerror(status));
        }
    } else {
        // It's a hostname, do a forward lookup
        if ((status = getaddrinfo(query, NULL, &hints, &res)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
            return 1;
        }

        printf("DNS lookup for %s:\n", query);

        for(p = res; p != NULL; p = p->ai_next) {
            void *addr;
            const char *ipver;

            // get the pointer to the address itself,
            // different fields in IPv4 and IPv6:
            if (p->ai_family == AF_INET) { // IPv4
                struct sockaddr_in *ipv4_res = (struct sockaddr_in *)p->ai_addr;
                addr = &(ipv4_res->sin_addr);
                ipver = "IPv4";
            } else { // IPv6
                struct sockaddr_in6 *ipv6_res = (struct sockaddr_in6 *)p->ai_addr;
                addr = &(ipv6_res->sin6_addr);
                ipver = "IPv6";
            }

            // convert the IP to a string and print it:
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
            printf("  %s: %s\n", ipver, ipstr);
        }

        freeaddrinfo(res); // free the linked list
    }

    return 0;
}
