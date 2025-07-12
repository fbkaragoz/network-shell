#include "../core/netan_core.h"
#include <stdio.h>
#include <string.h>
#include "../modules/ping.h"
#include "../modules/traceroute.h"
#include "../modules/scanner.h"
#include "../modules/dns.h"

int main(int argc, char *argv[]) {
    if (netan_init() != 0) {
        return 1;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [options]\n", argv[0]);
        netan_cleanup();
        return 1;
    }

    int result = 0;
    if (strcmp(argv[1], "ping") == 0) {
        result = ping_main(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "trace") == 0) {
        result = traceroute_main(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "scan") == 0) {
        result = scanner_main(argc - 1, argv + 1);
    } else if (strcmp(argv[1], "dns") == 0) {
        result = dns_main(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        result = 1;
    }

    netan_cleanup();
    return result;
}
