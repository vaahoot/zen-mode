#ifndef HOSTS_H
#define HOSTS_H

void flush_dns(void);
int block_domain(const char *domain);
int unblock_domain(const char *domain);

#endif
