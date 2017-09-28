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
    vfprintf(stdout, fmt, arg);
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
 * \brief Read/write the entire buffer from/to a socket.
 *
 **************************************************************************
 */
static int
ReadOrWriteFully(int sd,       // IN
                 void *buf,    // IN (write), OUT (read)
                 int nbytes,   // IN
                 bool isRead)  // IN
{
    int bytesLeft = nbytes;

    while (bytesLeft > 0) {
        int n = isRead ? read(sd, buf, bytesLeft) :
                         write(sd, buf, bytesLeft);
        if (n <= 0) {
            if (n < 0) {
                Error("%s error: %d\n", isRead ? "read" : "write", n);
            }
            return n;
        }
        bytesLeft -= n;
        buf += n;
    }
    return nbytes;
}


/**
 **************************************************************************
 *
 * \brief Read the entire buffer from the socket.
 *
 **************************************************************************
 */
int
ReadFully(int sd,      // IN
          void *buf,   // OUT
          int nbytes)  // IN
{
    return ReadOrWriteFully(sd, buf, nbytes, true);
}


/**
 **************************************************************************
 *
 * \brief Write the entire buffer to the socket.
 *
 **************************************************************************
 */
int
WriteFully(int sd,     // IN
           void *buf,  // IN
           int nbytes) // IN
{
    return ReadOrWriteFully(sd, buf, nbytes, false);
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
 * \brief Convert an IPv4/IPv6 socket address to a string of "ip:port".
 *
 **************************************************************************
 */
void
SocketAddrToString6(const struct sockaddr *addr,  // IN
                    char *addrStr,                // OUT
                    int addrStrLen)               // IN
{
    char ipAddrStr[INET6_ADDRSTRLEN];

    switch (addr->sa_family) {
        case AF_INET: {
            const struct sockaddr_in *a4 = (const struct sockaddr_in *)addr;
            inet_ntop(AF_INET, &a4->sin_addr, ipAddrStr, sizeof ipAddrStr);
            snprintf(addrStr, addrStrLen, "%s:%u",
                     ipAddrStr, ntohs(a4->sin_port));
            break;
        }
        case AF_INET6: {
            const struct sockaddr_in6 *a6 = (const struct sockaddr_in6 *)addr;
            inet_ntop(AF_INET6, &a6->sin6_addr, ipAddrStr, sizeof ipAddrStr);
            snprintf(addrStr, addrStrLen, "[%s]:%u",
                     ipAddrStr, ntohs(a6->sin6_port));
            break;
        }
        default:
            Error("Unknown address family: %d\n", addr->sa_family);
            exit(EXIT_FAILURE);
    }
}


/**
 **************************************************************************
 *
 * \brief Print out a given protocol message.
 *
 **************************************************************************
 */
void
PrintMsg(const MsgHdr *msg,   // IN
         const char *prefix)  // IN
{
    switch (msg->type) {
        case MSG_SHOW:
            Log("   %s Request: SHOW\n", prefix);
            break;
        case MSG_CLEAR:
            Log("   %s Request: CLEAR\n", prefix);
            break;
        case MSG_POST:
            Log("   %s Request: POST (%u bytes)\n", prefix, msg->dataSize);
            break;
        case MSG_BOARD:
            Log("   %s Reply: BOARD (%u bytes)\n", prefix, msg->dataSize);
            break;
        case MSG_STATUS:
            Log("   %s Reply: STATUS (%u)\n", prefix, msg->status);
            break;
        default:
            Log("   %s Unknown message type %d\n", prefix, msg->type);
    }
}
