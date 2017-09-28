#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <arpa/inet.h>

#include "common.h"


/**
 **************************************************************************
 *
 * \brief Log a message.
 *
 **************************************************************************
 */
void
Log(const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
}


/**
 **************************************************************************
 *
 * \brief Log an error message.
 *
 **************************************************************************
 */
void
Error(const char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    vfprintf(stderr, fmt, arg);
    va_end(arg);
}


/**
 **************************************************************************
 *
 * \brief Convert a socket endpoint address to a string of "ip:port".
 *
 **************************************************************************
 */
void
SocketAddrToString(const struct sockaddr_in *addr,  // IN
                   char *addrStr,                   // OUT
                   int addrStrLen)                  // IN
{
    char ipAddrStr[INET_ADDRSTRLEN];

    inet_ntop(AF_INET, &addr->sin_addr, ipAddrStr, sizeof ipAddrStr);
    snprintf(addrStr, addrStrLen, "%s:%u", ipAddrStr, ntohs(addr->sin_port));
}


/**
 **************************************************************************
 *
 * \brief Compute the time difference in microseconds between two timevals.
 *
 **************************************************************************
 */
int
timeval_sub(const struct timeval *tvEnd, const struct timeval *tvStart)
{
   int secs, usecs;

   if (tvEnd->tv_usec < tvStart->tv_usec) {
       secs  = tvEnd->tv_sec - tvStart->tv_sec;
       usecs = tvEnd->tv_usec - tvStart->tv_usec;
   } else {
       secs  = tvEnd->tv_sec - tvStart->tv_sec - 1;
       usecs = tvEnd->tv_usec + 1000000 - tvStart->tv_usec;
   }
   return secs * 1000000 + usecs;
}
