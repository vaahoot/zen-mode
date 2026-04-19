#include "hosts.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HOSTS_PATH "/etc/hosts"
#define HOSTS_TMP "/etc/hosts.zen.tmp"
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

// Returns 1 if `line` has a whitespace-delimited field equal to `domain`
// or to "www.<domain>".
static int line_matches_domain(const char *line, const char *domain) {
  size_t dlen = strlen(domain);
  const char *p = line;
  while (*p) {
    while (*p && isspace((unsigned char)*p))
      p++;
    const char *start = p;
    while (*p && !isspace((unsigned char)*p))
      p++;
    size_t flen = (size_t)(p - start);
    if (flen == dlen && memcmp(start, domain, dlen) == 0)
      return 1;
    if (flen == dlen + 4 && memcmp(start, "www.", 4) == 0 &&
        memcmp(start + 4, domain, dlen) == 0)
      return 1;
  }
  return 0;
}

void unblock_domain(const char *domain) {
  FILE *in = fopen(HOSTS_PATH, "r");
  if (!in) {
    perror("Failed to open /etc/hosts");
    return;
  }
  FILE *out = fopen(HOSTS_TMP, "w");
  if (!out) {
    perror("Failed to open temp file");
    fclose(in);
    return;
  }

  char buf[1024];
  int write_err = 0;
  while (fgets(buf, sizeof(buf), in)) {
    int drop =
        strstr(buf, MARKER) && (!domain || line_matches_domain(buf, domain));
    if (!drop && fputs(buf, out) == EOF) {
      write_err = 1;
      break;
    }
  }

  fclose(in);
  if (fclose(out) == EOF)
    write_err = 1;

  if (write_err) {
    perror("Failed to write /etc/hosts");
    unlink(HOSTS_TMP);
    return;
  }
  if (rename(HOSTS_TMP, HOSTS_PATH) != 0) {
    perror("Failed to replace /etc/hosts");
    unlink(HOSTS_TMP);
  }
}
