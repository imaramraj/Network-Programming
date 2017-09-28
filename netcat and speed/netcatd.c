#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"

/**
 * The server command line arguments.
 */
typedef struct ServerArgs {
    unsigned short listenPort;
} ServerArgs;

static int            listenSocket    = -1;
static volatile Bool  listenerRunning = TRUE;


/**
 **************************************************************************
 *
 * \brief Create a TCP listen socket on the given port.
 *
 **************************************************************************
 */
static int
CreatePassiveTCP(unsigned port)
{
    int msock;
    struct sockaddr_in svrAddr;

    msock = socket(AF_INET, SOCK_STREAM, 0);
    if (msock < 0) {
        perror("Failed to allocate the listen socket");
        exit(EXIT_FAILURE);
    }

    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family      = AF_INET;
    svrAddr.sin_port        = htons(port);
    svrAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(msock, (struct sockaddr *)&svrAddr, sizeof svrAddr) < 0) {
        perror("Failed to bind IP address and port to the listen socket");
        exit(EXIT_FAILURE);
    }

    if (listen(msock, 5) < 0) {
        perror("Failed to listen for connections");
        exit(EXIT_FAILURE);
    }

    return msock;
}


/**
 **************************************************************************
 *
 * \brief The server loop to handle requests from a particular client.
 *
 **************************************************************************
 */
void
Server(int sd,               // IN
       const char *cliName)  // IN
{
    int n;

    Log("\nClient %s connected\n", cliName);

    do {
        char ch;
        n = read(sd, &ch, sizeof ch);
        if (n > 0) {
            n = fwrite(&ch, 1, 1, stdout);
        }
    } while (n > 0);

    close(sd);
    Log("Client %s disconnected\n\n", cliName);
}


/**
 **************************************************************************
 *
 * \brief The server loop to accept new client connections.
 *
 **************************************************************************
 */
static void
ServerListenerLoop(void)
{
    while (listenerRunning) {
        int ssock;
        struct sockaddr_in cliAddr;
        struct sockaddr_in localAddr;
        socklen_t cliAddrLen;
        socklen_t localAddrLen;
        char cliName[INET_ADDRSTRLEN + PORT_STRLEN];
        char svrName[INET_ADDRSTRLEN + PORT_STRLEN];

        cliAddrLen = sizeof cliAddr;
        ssock = accept(listenSocket, (struct sockaddr *)&cliAddr, &cliAddrLen);
        if (ssock < 0) {
            if (listenerRunning && ssock != EINTR) {
                perror("Failed to accept a connection");
                listenerRunning = FALSE;
            }
            continue;
        }

        localAddrLen = sizeof localAddr;
        if (getsockname(ssock,
                        (struct sockaddr *)&localAddr,
                        &localAddrLen) < 0) {
            perror("Failed to get server address info for new connection");
            close(ssock);
            continue;
        }
        SocketAddrToString(&localAddr, svrName, sizeof svrName);
        SocketAddrToString(&cliAddr, cliName, sizeof cliName);
        Log("Accepted client %s at server %s\n", cliName, svrName);

        Server(ssock, cliName);
        listenerRunning = FALSE;
    }
}


/**
 **************************************************************************
 *
 * \brief Show the usage message and exit the program.
 *
 **************************************************************************
 */
static void
Usage(const char *prog) // IN
{
    Log("Usage:\n");
    Log("    %s <port>\n", prog);
    exit(EXIT_FAILURE);
}


/**
 **************************************************************************
 *
 * \brief Parse command line arguments.
 *
 **************************************************************************
 */
void
ParseArgs(int argc,            // IN
          char *argv[],        // IN
          ServerArgs *svrArgs) // OUT
{
    if (argc != 2) {
        Usage(argv[0]);
    }
    svrArgs->listenPort = atoi(argv[1]);
    if (svrArgs->listenPort == 0) {
        Usage(argv[0]);
    }
}


/**
 **************************************************************************
 *
 * \brief Main entry point.
 *
 **************************************************************************
 */
int
main(int argc,      // IN
     char *argv[])  // IN
{
    ServerArgs svrArgs;

    ParseArgs(argc, argv, &svrArgs);

    listenSocket = CreatePassiveTCP(svrArgs.listenPort);

    Log("\nServer started listening at *:%u\n", svrArgs.listenPort);

    ServerListenerLoop();

    close(listenSocket);
    Log("Server stopped listening at *:%u\n", svrArgs.listenPort);

    return 0;
}
