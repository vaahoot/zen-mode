#include "hosts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOSTS_PATH "/etc/hosts"
#define MARKER "# ZEN_BLOCK"

void flush_dns(void) {
    system("dscacheutil -flushcache");
    system("killall -HUP mDNSResponder");
}

void block_domain(const char *domain) {
    FILE *f = fopen(HOSTS_PATH, "a");
    if (!f) {
        perror("Failed to open /etc/hosts");
        return;
    }

    // Adding domain and www.domain to make sure it is blocked properly.
    fprintf(f, "127.0.0.1 %s %s\n", domain, MARKER);
    fprintf(f, "127.0.0.1 www.%s %s\n", domain, MARKER);

    fclose(f);
}

void unblock_domain(const char *domain) {
    FILE *f = fopen(HOSTS_PATH, "r");
    if (!f) {
        perror("Failed to open /etc/hosts");
        return;
    }

    char lines[512][256];
    int count = 0;
    while (fgets(lines[count], 256, f) && count < 512) {
        if (!strstr(lines[count], MARKER)) {
            count++;  // no marker, always keep
        } else if (domain && !strstr(lines[count], domain)) {
            count++;  // has marker but wrong domain, keep
        }
        // otherwise: skip the line (remove it)
    }
    fclose(f);

    f = fopen(HOSTS_PATH, "w");
    if (!f) {
        perror("Failed to write /etc/hosts");
        return;
    }
    for (int i = 0; i < count; i++) {
        fputs(lines[i], f);
    }
    fclose(f);
}
