#ifndef HOSTS_H
#define HOSTS_H

void flush_dns(void);
void block_domain(const char *domain);
void unblock_domain(const char *domain);

#endif
