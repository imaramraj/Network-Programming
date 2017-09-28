#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "common.h"
#include "spdtest.h"


/**
 * The server command line arguments.
 */
typedef struct ClientArgs {
    char          *svrHost;
    unsigned short svrPort;
    unsigned int   msgSize;
} ClientArgs;


/**
 **************************************************************************
 *
 * \brief Create a UDP socket on the given port.
 *
 **************************************************************************
 */
static int
CreateClientUDP(void)
{
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Failed to allocate the UDP socket");
        exit(EXIT_FAILURE);
    }
    return sock;
}


/**
 **************************************************************************
 *
 * \brief The server loop to process client requests.
 *
 **************************************************************************
 */
static void
Client(int sock, ClientArgs *cliArgs)
{
    int i;
    SpdTestMsg *msg;
    SpdTestAck ack;
    struct sockaddr_in svrAddr;
    struct hostent *phe;
    char svrName[INET_ADDRSTRLEN + PORT_STRLEN];
    struct timeval tvStart, tvEnd;
    int elapsedUS;

    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;
    svrAddr.sin_port   = htons(cliArgs->svrPort);

    phe = gethostbyname(cliArgs->svrHost);
    if (phe == NULL) {
        perror("Failed to look up hostname\n");
        exit(EXIT_FAILURE);
    }
    memcpy(&svrAddr.sin_addr, phe->h_addr_list[0], phe->h_length);

    SocketAddrToString(&svrAddr, svrName, sizeof svrName);
    Log("\nTesting with the server at %s\n", svrName);

    msg = (SpdTestMsg *)malloc(cliArgs->msgSize);
    if (msg == NULL) {
        Error("Cannot allocate memory for message receive buffer\n");
        exit(EXIT_FAILURE);
    }

    gettimeofday(&tvStart, NULL);

    for (i = 0; i < SPDTEST_NUM_ITERS; i++) {
        socklen_t alen = sizeof svrAddr;
        msg->hdr.seq = i + 1;
        int n = sendto(sock, msg, cliArgs->msgSize, 0,
                       (struct sockaddr *)&svrAddr, alen);
        if (n <= 0) {
            perror("Failed to write message");
            exit(EXIT_FAILURE);
        }
        n = recvfrom(sock, &ack, sizeof ack, 0,
                     (struct sockaddr *)&svrAddr, &alen);
        if (n <= 0) {
            perror("Failed to read message");
            exit(EXIT_FAILURE);
        }
        if (ack.seq != msg->hdr.seq) {
            Error("Mismatched message sequence number: msg %u ack %u\n",
                  msg->hdr.seq, ack.seq);
            exit(EXIT_FAILURE);
        }
    }
    free(msg);

    gettimeofday(&tvEnd, NULL);

    elapsedUS = timeval_sub(&tvEnd, &tvStart);
    Log("Message size: %8u bytes, elapsed: %9u us, bandwidth = %u Bps\n",
        cliArgs->msgSize, elapsedUS,
        SPDTEST_NUM_ITERS * cliArgs->msgSize * 1000 / (elapsedUS / 1000));
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
    Log("    %s server_ip server_port msg_size\n\n", prog);
    Log("where msg_size must be at least %u\n\n", sizeof(SpdTestHdr));
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
          ClientArgs *cliArgs) // OUT
{
    if (argc != 4) {
        Usage(argv[0]);
    }
    memset(cliArgs, 0, sizeof *cliArgs);

    cliArgs->svrHost = argv[1];
    cliArgs->svrPort = atoi(argv[2]);
    cliArgs->msgSize = atoi(argv[3]);

    if (cliArgs->svrPort == 0 || cliArgs->msgSize < sizeof(SpdTestHdr)) {
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
    int sock;
    ClientArgs cliArgs;
    ParseArgs(argc, argv, &cliArgs);

    sock = CreateClientUDP();

    Client(sock, &cliArgs);

    close(sock);
    return 0;
}
