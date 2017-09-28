#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "common.h"
#include "spdtest.h"


/**
 * The server command line arguments.
 */
typedef struct ServerArgs {
    unsigned short port;
    unsigned int   msgSize;
    unsigned int   sleepUS;
} ServerArgs;

static int svrSock = -1;


/**
 **************************************************************************
 *
 * \brief Create a UDP socket on the given port.
 *
 **************************************************************************
 */
static int
CreatePassiveUDP(unsigned port)
{
    int msock;
    struct sockaddr_in svrAddr;

    msock = socket(AF_INET, SOCK_DGRAM, 0);
    if (msock < 0) {
        perror("Failed to allocate the UDP socket");
        exit(EXIT_FAILURE);
    }

    memset(&svrAddr, 0, sizeof svrAddr);
    svrAddr.sin_family      = AF_INET;
    svrAddr.sin_port        = htons(port);
    svrAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(msock, (struct sockaddr *)&svrAddr, sizeof svrAddr) < 0) {
        perror("Failed to bind IP address and port to the listen socket");
        exit(EXIT_FAILURE);
    }

    return msock;
}


/**
 **************************************************************************
 *
 * \brief The server loop to process client requests.
 *
 **************************************************************************
 */
static void
ServerLoop(ServerArgs *svrArgs)
{
    int i;
    SpdTestMsg *msg;
    SpdTestAck *ack;

    if (svrArgs->msgSize < sizeof *ack) {
        Error("msgSize is too small");
        return;
    }

    msg = (SpdTestMsg *)malloc(svrArgs->msgSize);
    if (msg == NULL) {
        Error("Cannot allocate memory for message receive buffer");
        return;
    }

    for (i = 0; i < SPDTEST_NUM_ITERS; i++) {
        struct sockaddr_in cliAddr;
        socklen_t alen = sizeof cliAddr;
        int n = recvfrom(svrSock, msg, svrArgs->msgSize, 0,
                         (struct sockaddr *)&cliAddr, &alen);
        if (n <= 0) {
            perror("Failed to read message");
            break;
        }

        Log("Received message %u\n", msg->hdr.seq);
        usleep(svrArgs->sleepUS);

        ack = &msg->hdr;
        n = sendto(svrSock, ack, sizeof *ack, 0,
                   (struct sockaddr *)&cliAddr, alen);
        if (n <= 0) {
            perror("Failed to write message");
            break;
        }
    }
    free(msg);
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
    Log("Usage:\n\n");
    Log("    %s port msg_size sleep_ms\n\n", prog);
    Log("where msg_size must be at least %u.\n\n", sizeof(SpdTestHdr));
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
    if (argc != 4) {
        Usage(argv[0]);
    }
    svrArgs->port    = atoi(argv[1]);
    svrArgs->msgSize = atoi(argv[2]);
    svrArgs->sleepUS = atoi(argv[3]) * 1000;

    if (svrArgs->port == 0 || svrArgs->msgSize < sizeof(SpdTestHdr) ||
        svrArgs->sleepUS == 0) {
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

    svrSock = CreatePassiveUDP(svrArgs.port);

    Log("\nServer started at *:%u\n", svrArgs.port);

    ServerLoop(&svrArgs);

    close(svrSock);
    Log("Server stopped at *:%u\n", svrArgs.port);

    return 0;
}
