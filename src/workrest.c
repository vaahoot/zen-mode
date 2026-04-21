#include "workrest.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t stop_requested = 0;

void notify(const char *message) {
  char cmd[512];
  int n = snprintf(cmd, sizeof(cmd),
                   "osascript -e 'display notification \"%s\" with title "
                   "\"Zen\"'",
                   message);
  if (n > 0 && (size_t)n < sizeof(cmd))
    system(cmd);
  system("afplay /System/Library/Sounds/Glass.aiff &");
}

int interruptible_sleep(unsigned seconds) {
  unsigned remaining = seconds;
  while (remaining > 0 && !stop_requested)
    remaining = sleep(remaining);
  return !stop_requested;
}

long parse_minutes(const char *s) {
  char *end;
  errno = 0;
  long v = strtol(s, &end, 10);
  if (errno || *end || v <= 0)
    return -1;
  return v;
}
