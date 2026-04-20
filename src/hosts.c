#include "hosts.h"
#include <ctype.h>
#include <errno.h>
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

// Returns 1 if `line` has a whitespace-delimited field exactly equal to `name`.
static int line_has_field(const char *line, const char *name) {
  size_t nlen = strlen(name);
  const char *p = line;
  while (*p) {
    while (*p && isspace((unsigned char)*p))
      p++;
    const char *start = p;
    while (*p && !isspace((unsigned char)*p))
      p++;
    size_t flen = (size_t)(p - start);
    if (flen == nlen && memcmp(start, name, nlen) == 0)
      return 1;
  }
  return 0;
}

// Returns 1 if `line` contains either `domain` or "www.<domain>" as a field.
static int line_matches_domain(const char *line, const char *domain) {
  if (line_has_field(line, domain))
    return 1;
  char www[512];
  int n = snprintf(www, sizeof(www), "www.%s", domain);
  if (n <= 0 || (size_t)n >= sizeof(www))
    return 0;
  return line_has_field(line, www);
}

// Checks which of `bare` / `www_form` already have a ZEN_BLOCK entry.
// Sets *have_bare and *have_www; returns 0 on success, -1 on I/O error
// (a missing file is treated as "neither is blocked").
static int check_existing_blocks(const char *bare, const char *www_form,
                                 int *have_bare, int *have_www) {
  *have_bare = 0;
  *have_www = 0;
  FILE *f = fopen(HOSTS_PATH, "r");
  if (!f)
    return errno == ENOENT ? 0 : -1;
  char buf[1024];
  while (fgets(buf, sizeof(buf), f)) {
    if (!strstr(buf, MARKER))
      continue;
    if (!*have_bare && line_has_field(buf, bare))
      *have_bare = 1;
    if (!*have_www && line_has_field(buf, www_form))
      *have_www = 1;
    if (*have_bare && *have_www)
      break;
  }
  fclose(f);
  return 0;
}

int block_domain(const char *domain) {
  char www[512];
  int n = snprintf(www, sizeof(www), "www.%s", domain);
  if (n <= 0 || (size_t)n >= sizeof(www)) {
    fprintf(stderr, "Failed to block %s: domain too long\n", domain);
    return -1;
  }

  int have_bare, have_www;
  if (check_existing_blocks(domain, www, &have_bare, &have_www) != 0) {
    fprintf(stderr, "Failed to block %s: %s\n", domain, strerror(errno));
    return -1;
  }
  if (have_bare && have_www) {
    printf("%s is already blocked.\n", domain);
    return 0;
  }

  FILE *f = fopen(HOSTS_PATH, "a");
  if (!f) {
    fprintf(stderr, "Failed to block %s: %s\n", domain, strerror(errno));
    return -1;
  }

  int ok = 1;
  if (!have_bare)
    ok &= fprintf(f, "127.0.0.1 %s %s\n", domain, MARKER) > 0;
  if (!have_www)
    ok &= fprintf(f, "127.0.0.1 %s %s\n", www, MARKER) > 0;
  if (fclose(f) == EOF)
    ok = 0;

  if (!ok) {
    fprintf(stderr, "Failed to block %s\n", domain);
    return -1;
  }
  return 0;
}

int unblock_domain(const char *domain) {
  FILE *in = fopen(HOSTS_PATH, "r");
  if (!in) {
    fprintf(stderr, "Failed to open /etc/hosts: %s\n", strerror(errno));
    return -1;
  }
  FILE *out = fopen(HOSTS_TMP, "w");
  if (!out) {
    fprintf(stderr, "Failed to open temp file: %s\n", strerror(errno));
    fclose(in);
    return -1;
  }

  char buf[1024];
  int write_err = 0;
  int dropped = 0;
  while (fgets(buf, sizeof(buf), in)) {
    int drop =
        strstr(buf, MARKER) && (!domain || line_matches_domain(buf, domain));
    if (drop) {
      dropped++;
      continue;
    }
    if (fputs(buf, out) == EOF) {
      write_err = 1;
      break;
    }
  }

  fclose(in);
  if (fclose(out) == EOF)
    write_err = 1;

  if (write_err) {
    fprintf(stderr, "Failed to write /etc/hosts: %s\n", strerror(errno));
    unlink(HOSTS_TMP);
    return -1;
  }

  if (domain && dropped == 0) {
    unlink(HOSTS_TMP);
    printf("%s is not blocked.\n", domain);
    return 0;
  }

  if (rename(HOSTS_TMP, HOSTS_PATH) != 0) {
    fprintf(stderr, "Failed to replace /etc/hosts: %s\n", strerror(errno));
    unlink(HOSTS_TMP);
    return -1;
  }
  return 0;
}
