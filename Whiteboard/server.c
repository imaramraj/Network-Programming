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
#include "server.h"

typedef struct WhiteBoard {
    int  dataSize;
    char dataBuf[MAX_BOARD_DATA_SIZE];
} WhiteBoard;

static WhiteBoard board;

typedef bool (*MsgFunc)(int sd, const MsgHdr *req, const char *cliName);

typedef struct MsgHandler {
    MsgType     type;
    MsgFunc     func;
} MsgHandler;

static bool ProcessMsgShow(int sd, const MsgHdr *req, const char *cliName);
static bool ProcessMsgClear(int sd, const MsgHdr *req, const char *cliName);
static bool ProcessMsgPost(int sd, const MsgHdr *req, const char *cliName);

MsgHandler msgHandlers[] = {
    { MSG_SHOW,  ProcessMsgShow  },
    { MSG_CLEAR, ProcessMsgClear },
    { MSG_POST,  ProcessMsgPost  },
};


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
 * \brief Handler for MSG_SHOW.
 *
 **************************************************************************
 */
static bool
ProcessMsgShow(int sd,               // IN
               const MsgHdr *req,    // IN
               const char *cliName)  // IN
{
    MsgHdr reply;

    PrintMsg(req, cliName);

    memset(&reply, 0, sizeof reply);
    reply.type     = MSG_BOARD;
    reply.dataSize = board.dataSize;

    if (WriteFully(sd, &reply, sizeof reply) <= 0) {
        return false;
    }
    if (WriteFully(sd, board.dataBuf, board.dataSize) <= 0) {
        return false;
    }

    PrintMsg(&reply, cliName);
    return true;
}


/**
 **************************************************************************
 *
 * \brief Handler for MSG_CLEAR.
 *
 **************************************************************************
 */
static bool
ProcessMsgClear(int sd,               // IN
                const MsgHdr *req,    // IN
                const char *cliName)  // IN
{
    MsgHdr reply;

    PrintMsg(req, cliName);

    memset(&reply, 0, sizeof reply);
    reply.type     = MSG_STATUS;
    reply.status   = MSG_STATUS_SUCCESS;
    reply.dataSize = 0;

    board.dataSize = 0;

    if (WriteFully(sd, &reply, sizeof reply) <= 0) {
        return false;
    }

    PrintMsg(&reply, cliName);
    return true;
}


/**
 **************************************************************************
 *
 * \brief Handler for MSG_POST.
 *
 **************************************************************************
 */
static bool
ProcessMsgPost(int sd,               // IN
               const MsgHdr *req,    // IN
               const char *cliName)  // IN
{
    MsgHdr reply;
    int bytesToStore, bytesToSkip;

    PrintMsg(req, cliName);

    bytesToStore = MIN(req->dataSize,
                       MAX_BOARD_DATA_SIZE - board.dataSize - 1);
    bytesToSkip = req->dataSize - bytesToStore;

    if (bytesToStore > 0) {
        if (ReadFully(sd, board.dataBuf + board.dataSize, bytesToStore) <= 0) {
            return false;
        }
        board.dataSize += bytesToStore;

        /* Always append a newline. */
        board.dataBuf[board.dataSize] = '\n';
        board.dataSize++;
    }

    while (bytesToSkip > 0) {
        char ch;
        if (ReadFully(sd, &ch, 1) <= 0) {
            return false;
        }
        bytesToSkip--;
    }

    memset(&reply, 0, sizeof reply);
    reply.type     = MSG_STATUS;
    reply.status   = MSG_STATUS_SUCCESS;
    reply.dataSize = 0;

    if (WriteFully(sd, &reply, sizeof reply) <= 0) {
        return false;
    }

    PrintMsg(&reply, cliName);
    return true;
}


/**
 **************************************************************************
 *
 * \brief The server routine to handle requests from a particular client.
 *
 **************************************************************************
 */
void
Server(int sd)  // IN
{
    struct sockaddr_storage cliAddr;
    char cliName[INET6_ADDRSTRLEN + PORT_STRLEN];
    socklen_t cliAddrLen;
    MsgHdr req;
    int i;

    cliAddrLen = sizeof cliAddr;
    if (getpeername(sd, (struct sockaddr *)&cliAddr, &cliAddrLen) < 0) {
        perror("Failed to get peer address info for client socket");
        close(sd);
        return;
    }
    SocketAddrToString6((const struct sockaddr *)&cliAddr,
                        cliName, sizeof cliName);

    Log("\nClient %s (sock=%u) connected\n", cliName, sd);

    if (ReadFully(sd, &req, sizeof(req)) > 0) {
        for (i = 0; i < ARRAYSIZE(msgHandlers); i++) {
            MsgHandler *handler = &msgHandlers[i];
            if (handler->type == req.type) {
                handler->func(sd, &req, cliName);
                break;
            }
        }
        if (i == ARRAYSIZE(msgHandlers)) {
            Error("   [%s] Unknown message type %d\n", cliName, req.type);
        }
    }

    Log("Client %s (sock=%u) disconnected\n\n", cliName, sd);
    close(sd);
}
