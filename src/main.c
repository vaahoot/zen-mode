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

int process_domains(int i, int argc, char **argv,
                    void (*action)(const char *)) {
  i++;
  if (i >= argc || argv[i][0] == '-') {
    fprintf(stderr, "Expected domains after %s\n", argv[i - 1]);
    return -1;
  }
  while (i < argc && argv[i][0] != '-') {
    action(argv[i]);
    i++;
  }
  return i - 1;
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

  for (int i = 1; i < argc; i++) {
    if (is_flag(argv[i], "-b", "--block")) {
      i = process_domains(i, argc, argv, block_domain);
      if (i == -1)
        return 1;
      printf("Blocked domains successfully.\n");
    } else if (is_flag(argv[i], "-u", "--unblock")) {
      i = process_domains(i, argc, argv, unblock_domain);
      if (i == -1)
        return 1;
      printf("Unblocked domains successfully.\n");
    } else if (is_flag(argv[i], "-U", "--unblock-all")) {
      unblock_domain(NULL);
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
