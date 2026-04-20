#include "hosts.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_help(void) {
  printf("Usage: zen [options]\n");
  printf("  -b, --block         block domains\n");
  printf("  -u, --unblock       unblock domains\n");
  printf("  -U, --unblock-all   remove all blocks\n");
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
  if (argc < 2) {
    print_help();
    return 0;
  }

  int had_failure = 0;
  for (int i = 1; i < argc; i++) {
    int (*action)(const char *) = NULL;
    if (is_flag(argv[i], "-b", "--block")) {
      action = block_domain;
    } else if (is_flag(argv[i], "-u", "--unblock")) {
      action = unblock_domain;
    } else if (is_flag(argv[i], "-U", "--unblock-all")) {
      if (unblock_domain(NULL) != 0)
        had_failure = 1;
      continue;
    } else if (is_flag(argv[i], "-h", "--help")) {
      print_help();
      return 0;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      return 1;
    }

    const char *flag = argv[i];
    if (i + 1 >= argc || argv[i + 1][0] == '-') {
      fprintf(stderr, "Expected domains after %s\n", flag);
      return 1;
    }
    while (i + 1 < argc && argv[i + 1][0] != '-') {
      i++;
      if (action(argv[i]) != 0)
        had_failure = 1;
    }
  }

  flush_dns();
  if (had_failure) {
    fprintf(stderr, "Some operations failed.\n");
    return 1;
  }
  printf("Done.\n");
  return 0;
}
