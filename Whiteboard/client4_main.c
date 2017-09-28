#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "client.h"


/**
 **************************************************************************
 *
 * \brief Create a client TCP socket and connect to the server.
 *
 **************************************************************************
 */
int
CreateClientTCP(const char *svrHost,     // IN
                unsigned short svrPort,  // IN
                char *svrName,           // OUT
                int svrNameLen)          // IN
{
    int sock;
    struct sockaddr_in svrAddr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to allocate the client socket");
        exit(EXIT_FAILURE);
    }

    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_port   = htons(svrPort);

    if (inet_pton(AF_INET, svrHost, &svrAddr.sin_addr.s_addr) <= 0) {
        perror("Failed to convert IP address\n");
        exit(EXIT_FAILURE);
    }

    SocketAddrToString(&svrAddr, svrName, svrNameLen);
    Log("Attempting %s\n", svrName);

    if (connect(sock, (struct sockaddr *)&svrAddr, sizeof(svrAddr)) < 0) {
        perror("Failed to connect to the server");
        exit(EXIT_FAILURE);
    }

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
    char svrName[INET_ADDRSTRLEN + PORT_STRLEN];

    ParseArgs(argc, argv, &cliArgs);

    sock = CreateClientTCP(cliArgs.svrHost, cliArgs.svrPort,
                           svrName, sizeof svrName);

    Log("Connected to server at %s\n", svrName);

    Client(sock, &cliArgs);

    close(sock);
    Log("\nDisconnected from server at %s\n", svrName);
    return 0;
}
