#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common.h"

/**
 * The client command line arguments.
 */
typedef struct ClientArgs {
    const char     *svrHost;
    unsigned short  svrPort;
} ClientArgs;


/**
 **************************************************************************
 *
 * \brief Create a client TCP socket and connect to the server.
 *
 **************************************************************************
 */
static int
CreateClientTCP(const char *svrHost,     // IN
                unsigned short svrPort,  // IN
                char *svrName,           // OUT
                int svrNameLen)          // IN
{
    int sock;
    struct sockaddr_in svrAddr;
    struct hostent *phe;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to allocate the client socket");
        exit(EXIT_FAILURE);
    }

    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_port   = htons(svrPort);

    phe = gethostbyname(svrHost);
    if (phe == NULL) {
        perror("Failed to look up hostname\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&svrAddr.sin_addr, phe->h_addr_list[0], phe->h_length);

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
 * \brief Print the usage message and exit the program.
 *
 **************************************************************************
 */
static void
Usage(const char *prog) // IN
{
    Log("Usage:\n");
    Log("    %s <server_ip> <server_port>\n", prog);
    exit(EXIT_FAILURE);
}


/**
 **************************************************************************
 *
 * \brief Parse the client command line arguments.
 *
 **************************************************************************
 */
void
ParseArgs(int argc,             // IN
          char *argv[],         // IN
          ClientArgs *cliArgs)  // OUT
{
    if (argc < 3) {
        Usage(argv[0]);
    }

    memset(cliArgs, 0, sizeof *cliArgs);

    cliArgs->svrHost = argv[1];
    cliArgs->svrPort = atoi(argv[2]);
    if (cliArgs->svrPort == 0) {
        Usage(argv[0]);
    }
}


/**
 **************************************************************************
 *
 * \brief Dispatch client commands.
 *
 **************************************************************************
 */
void
Client(int sock)                   // IN
{
    int n;
    do {
        char ch;
        n = fread(&ch, 1, 1, stdin);
        if (n > 0) {
            n = write(sock, &ch, 1);
        }
    } while (n > 0);
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

    Client(sock);

    close(sock);
    Log("Disconnected from server at %s\n", svrName);
    return 0;
}
