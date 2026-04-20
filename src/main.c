#include "hosts.h"
#include "workrest.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_help(void) {
  printf("Usage: zen [options]\n");
  printf("  -b, --block         block domains (runs until interrupted)\n");
  printf("  -u, --unblock       unblock domains\n");
  printf("  -U, --unblock-all   remove all blocks\n");
  printf("  -w, --work MINUTES  work phase duration (requires -r)\n");
  printf("  -r, --rest MINUTES  rest phase duration (requires -w)\n");
  printf("  -h, --help          show this message\n");
}

int is_flag(const char *arg, const char *short_f, const char *long_f) {
  return strcmp(arg, short_f) == 0 || strcmp(arg, long_f) == 0;
}

static void on_signal(int sig) {
  (void)sig;
  stop_requested = 1;
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

  const char **blocked = calloc((size_t)argc, sizeof(char *));
  if (!blocked) {
    fprintf(stderr, "Out of memory.\n");
    return 1;
  }
  size_t blocked_count = 0;
  long work_minutes = 0, rest_minutes = 0;

  int had_failure = 0;
  for (int i = 1; i < argc; i++) {
    int (*action)(const char *) = NULL;
    int is_block = 0;
    if (is_flag(argv[i], "-b", "--block")) {
      action = block_domain;
      is_block = 1;
    } else if (is_flag(argv[i], "-u", "--unblock")) {
      action = unblock_domain;
    } else if (is_flag(argv[i], "-U", "--unblock-all")) {
      if (unblock_domain(NULL) != 0)
        had_failure = 1;
      continue;
    } else if (is_flag(argv[i], "-w", "--work") ||
               is_flag(argv[i], "-r", "--rest")) {
      int is_work = is_flag(argv[i], "-w", "--work");
      if (i + 1 >= argc) {
        fprintf(stderr, "Expected minutes after %s\n", argv[i]);
        free(blocked);
        return 1;
      }
      long v = parse_minutes(argv[++i]);
      if (v < 0) {
        fprintf(stderr, "Invalid minutes value: %s\n", argv[i]);
        free(blocked);
        return 1;
      }
      if (is_work)
        work_minutes = v;
      else
        rest_minutes = v;
      continue;
    } else if (is_flag(argv[i], "-h", "--help")) {
      print_help();
      free(blocked);
      return 0;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      free(blocked);
      return 1;
    }

    const char *flag = argv[i];
    if (i + 1 >= argc || argv[i + 1][0] == '-') {
      fprintf(stderr, "Expected domains after %s\n", flag);
      free(blocked);
      return 1;
    }
    while (i + 1 < argc && argv[i + 1][0] != '-') {
      i++;
      if (action(argv[i]) != 0) {
        had_failure = 1;
      } else if (is_block) {
        blocked[blocked_count++] = argv[i];
      }
    }
  }

  flush_dns();

  if ((work_minutes > 0) != (rest_minutes > 0)) {
    fprintf(stderr, "--work and --rest must be used together.\n");
    free(blocked);
    return 1;
  }
  if (work_minutes > 0 && blocked_count == 0) {
    fprintf(stderr, "--work/--rest require -b with at least one domain.\n");
    free(blocked);
    return 1;
  }

  if (blocked_count == 0) {
    free(blocked);
    if (had_failure) {
      fprintf(stderr, "Some operations failed.\n");
      return 1;
    }
    printf("Done.\n");
    return 0;
  }

  struct sigaction sa;
  sa.sa_handler = on_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);

  int currently_blocked = 1;

  if (work_minutes > 0) {
    printf("Work/rest cycle: %ld min work, %ld min rest. Ctrl-C to stop.\n",
           work_minutes, rest_minutes);
    while (!stop_requested) {
      char msg[128];
      if (!interruptible_sleep((unsigned)work_minutes * 60))
        break;

      for (size_t i = 0; i < blocked_count; i++) {
        if (unblock_domain(blocked[i]) != 0)
          had_failure = 1;
      }
      flush_dns();
      currently_blocked = 0;
      snprintf(msg, sizeof(msg), "Break time — rest for %ld minutes.",
               rest_minutes);
      printf("%s\n", msg);
      notify(msg);

      if (!interruptible_sleep((unsigned)rest_minutes * 60))
        break;

      for (size_t i = 0; i < blocked_count; i++) {
        if (block_domain(blocked[i]) != 0)
          had_failure = 1;
      }
      flush_dns();
      currently_blocked = 1;
      snprintf(msg, sizeof(msg), "Back to work — focus for %ld minutes.",
               work_minutes);
      printf("%s\n", msg);
      notify(msg);
    }
  } else {
    printf("Blocking %zu domain(s). Press Ctrl-C to unblock and exit.\n",
           blocked_count);
    while (!stop_requested)
      pause();
  }

  printf("\nUnblocking...\n");
  if (currently_blocked) {
    for (size_t i = 0; i < blocked_count; i++) {
      if (unblock_domain(blocked[i]) != 0)
        had_failure = 1;
    }
    flush_dns();
  }
  free(blocked);

  if (had_failure) {
    fprintf(stderr, "Some operations failed.\n");
    return 1;
  }
  printf("Done.\n");
  return 0;
}
