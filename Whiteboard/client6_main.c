#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common.h"
#include "client.h"


/**
 **************************************************************************
 *
 * \brief Bind a link-local IPv6 socket to an eth device.
 *
 **************************************************************************
 */
static bool
BindLinkLocalToDevice(int sock,                    // IN
                      struct sockaddr_in6 *addr6,  // IN
                      const char *svrName)         // IN
{
    if (IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr)) {
        char bindDev[] = "eth0";
        Log("Link-local: %s, binding to %s\n", svrName, bindDev);
        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,
                       bindDev, sizeof bindDev) < 0) {
            perror("Failed to bind to device");
            return false;
        }
    }
    return true;
}


/**
 **************************************************************************
 *
 * \brief Create a client TCP socket and connect to the server.
 *
 **************************************************************************
 */
int
CreateClientTCP6(const char *svrHost,     // IN
                 unsigned short svrPort,  // IN
                 char *svrName,           // OUT
                 int svrNameLen)          // IN
{
    int sock = -1;
    int s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    char portStr[PORT_STRLEN];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_INET6;
    hints.ai_flags    = AI_V4MAPPED;
    hints.ai_socktype = SOCK_STREAM;

    snprintf(portStr, sizeof portStr, "%d", svrPort);

    s = getaddrinfo(svrHost, portStr, &hints, &result);
    if (s != 0) {
        Error("getaddrinfo %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)rp->ai_addr;

        SocketAddrToString6(rp->ai_addr, svrName, svrNameLen);
        Log("Attempting %s\n", svrName);

        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0) {
            continue;
        }

        if (!BindLinkLocalToDevice(sock, addr6, svrName)) {
            continue;
        }

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) >= 0) {
            break;   /* Success */
        }
        close(sock);
    }

    if (rp == NULL) {   /*  No address succeeded */
        Error("Could not connect to [%s]:%d\n", svrHost, svrPort);
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);   /* No longer needed */
    return sock;
}


/**
 **************************************************************************
 *
 * \brief Main entry point.
 *
 **************************************************************************
 */
int
main(int argc, char *argv[])
{
    int sock;
    ClientArgs cliArgs;
    char svrName[INET6_ADDRSTRLEN + PORT_STRLEN];

    ParseArgs(argc, argv, &cliArgs);

    sock = CreateClientTCP6(cliArgs.svrHost, cliArgs.svrPort,
                            svrName, sizeof svrName);

    Log("Connected to server at %s\n", svrName);

    Client(sock, &cliArgs);

    close(sock);
    Log("\nDisconnected from server at %s\n", svrName);
    return 0;
}
