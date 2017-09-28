#ifndef _COMMON_H_
#define _COMMON_H_

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define ARRAYSIZE(_x)    (sizeof(_x) / sizeof((_x)[0]))

#define FALSE  0
#define TRUE   1

typedef unsigned char Bool;

#define PORT_STRLEN      6
#define MAX_TITLE_LEN    32

void Log(const char *fmt, ...);
void Error(const char *fmt, ...);

void SocketAddrToString(const struct sockaddr_in *addr, char *addrStr,
                        int addrStrLen);

int
timeval_sub(const struct timeval *tvEnd, const struct timeval *tvStart);

#endif
