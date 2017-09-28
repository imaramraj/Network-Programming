#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "common.h"
#include "client.h"

typedef bool (*CmdFunc)(int sd, char *data, int dataSize);

typedef struct CmdHandler {
    const char *cmd;
    CmdFunc     func;
} CmdHandler;

static bool ProcessCmdHelp(int sd, char *data, int dataSize);
static bool ProcessCmdShow(int sd, char *data, int dataSize);
static bool ProcessCmdClear(int sd, char *data, int dataSize);
static bool ProcessCmdPost(int sd, char *data, int dataSize);

CmdHandler cmdHandlers[] = {
    { "help",  ProcessCmdHelp  },
    { "show",  ProcessCmdShow  },
    { "clear", ProcessCmdClear },
    { "post",  ProcessCmdPost  },
};


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
 * \brief Process the "help" command.
 *
 **************************************************************************
 */
static bool
ProcessCmdHelp(int sd,        // IN
               char *data,    // IN
               int dataSize)  // IN
{
    printf("Commands:\n");
    printf("   help          : Display this screen.\n");
    printf("   show          : Show the content of White Board.\n");
    printf("   clear         : Clear the content of White Board.\n");
    printf("   post message  : Post a message (\"msg\") to White Board.\n");
    printf("\n");
    return true;
}


/**
 **************************************************************************
 *
 * \brief Process the "show" command.
 *
 **************************************************************************
 */
static bool
ProcessCmdShow(int sd,        // IN
               char *data,    // IN
               int dataSize)  // IN
{
    MsgHdr req, reply;

    memset(&req, 0, sizeof req);
    req.type = MSG_SHOW;

    if (WriteFully(sd, &req, sizeof req) <= 0) {
        return false;
    }

    if (ReadFully(sd, &reply, sizeof reply) <= 0) {
        return false;
    }
    if (reply.type != MSG_BOARD) {
        Error("Unexpected reply message type %d\n", reply.type);
        return false;
    }

    while (reply.dataSize > 0) {
        char ch;
        if (ReadFully(sd, &ch, sizeof ch) <= 0) {
            return false;
        }
        printf("%c", ch);
        reply.dataSize--;
    }
    return false;
}


/**
 **************************************************************************
 *
 * \brief Process the "clear" command.
 *
 **************************************************************************
 */
static bool
ProcessCmdClear(int sd,        // IN
                char *data,    // IN
                int dataSize)  // IN
{
    MsgHdr req, reply;

    memset(&req, 0, sizeof req);
    req.type = MSG_CLEAR;

    if (WriteFully(sd, &req, sizeof req) <= 0) {
        return false;
    }
    if (ReadFully(sd, &reply, sizeof reply) <= 0) {
        return false;
    }
    if (reply.type != MSG_STATUS) {
        Error("Unexpected reply message type %d\n", reply.type);
        return false;
    }
    return false;
}


/**
 **************************************************************************
 *
 * \brief Process the "post" command.
 *
 **************************************************************************
 */
static bool
ProcessCmdPost(int sd,        // IN
               char *data,    // IN
               int dataSize)  // IN
{
    MsgHdr req, reply;

    memset(&req, 0, sizeof req);
    req.type     = MSG_POST;
    req.dataSize = dataSize;

    if (WriteFully(sd, &req, sizeof req) <= 0) {
        return false;
    }
    if (WriteFully(sd, data, dataSize) <= 0) {
        return false;
    }

    if (ReadFully(sd, &reply, sizeof reply) <= 0) {
        return false;
    }
    if (reply.type != MSG_STATUS) {
        Error("Unexpected reply message type %d\n", reply.type);
        return false;
    }
    return false;
}


/**
 **************************************************************************
 *
 * \brief Dispatch client commands.
 *
 **************************************************************************
 */
void
Client(int sock,                   // IN
       const ClientArgs *cliArgs)  // IN
{
    bool running = true;

    Log("\n*** Welcome to 207 White Board Client. *** \n\n");
    Log("Enter a command or 'help' to see a list of available commands.\n\n");

    while (running) {
        char *cmdBuf;
        char *cmd, *saveptr;
        char *data;
        int cmdBufSize, dataSize;
        int i;

        cmdBuf = readline("207> ");
        if (cmdBuf == NULL) {
            return;
        }

        cmdBufSize = strlen(cmdBuf);

        cmd = strtok_r(cmdBuf, " ", &saveptr);

        data     = cmdBuf + strlen(cmd) + 1;
        dataSize = cmdBufSize - strlen(cmd);

        for (i = 0; i < ARRAYSIZE(cmdHandlers); i++) {
            CmdHandler *handler = &cmdHandlers[i];
            if (strcasecmp(cmd, handler->cmd) == 0) {
                running = handler->func(sock, data, dataSize);
                break;
            }
        }
        if (i == ARRAYSIZE(cmdHandlers)) {
            Error("Unknown command %s\n", cmd);
            running = false;
        }
        free(cmdBuf);
    }
}
