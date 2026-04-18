#include "hosts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_help(void) {
  printf("Usage: zen [options]\n");
  printf("  -b, --block         list domains to block\n");
  printf("  -u, --unblock-all   remove all blocks\n");
  printf("  -h, --help          show this message\n");
}

int is_flag(const char *arg, const char *short_f, const char *long_f) {
  return strcmp(arg, short_f) == 0 || strcmp(arg, long_f) == 0;
}

int main(int argc, char **argv) {
  if (getuid() != 0) {
    fprintf(stderr, "zen requires root privileges. Run with sudo.\n");
    return 1;
  }
  for (int i = 1; i < argc; i++) {
    if (is_flag(argv[i], "-b", "--block")) {
      i++;
      while (i < argc && argv[i][0] != '-') {
        block_domain(argv[i]);
        i++;
      }
      i--;
      printf("Blocked domains successfully.\n");
    } else if (is_flag(argv[i], "-u", "--unblock-all")) {
      unblock_all();
      printf("Unblocked domains successfully.\n");
    } else if (is_flag(argv[i], "-h", "--help")) {
      print_help();
      return 0;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      return 1;
    }
  }

  flush_dns();
  return 0;
}
