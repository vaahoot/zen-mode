#ifndef WORKREST_H
#define WORKREST_H

#include <signal.h>

extern volatile sig_atomic_t stop_requested;

void notify(const char *message);

// Sleep for `seconds`, returning early if stop_requested is set.
// Returns 1 if completed, 0 if interrupted.
int interruptible_sleep(unsigned seconds);

// Parses a positive integer minutes value from `s`.
// Returns -1 on failure.
long parse_minutes(const char *s);

#endif
