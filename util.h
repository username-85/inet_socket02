#ifndef UTIL_H
#define UTIL_H

#include <sys/socket.h>
#include <netdb.h>

int inet_connect(const char *host, const char *service, int type);
int inet_listen(const char *service, int backlog, socklen_t *addrlen);
int inet_bind(const char *service, int type, socklen_t *addrlen);
char * inet_addr_str(const struct sockaddr *addr, socklen_t addrlen,
                     char *addr_str, int addr_str_len);

#endif
