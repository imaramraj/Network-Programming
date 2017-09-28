#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ARRAYSIZE(_x)    (sizeof(_x) / sizeof((_x)[0]))
#define MIN(x, y)        (((x) <= (y)) ? (x) : (y))

#define PORT_STRLEN      6
#define MAX_TITLE_LEN    32

#define MAX_BOARD_DATA_SIZE 8192

/**
 *  Message type exchanged between client/server.
 */
typedef enum MsgType {
    MSG_UNKNOWN = 0,
    /* Client -> Server */
    MSG_SHOW    = 1,
    MSG_CLEAR   = 2,
    MSG_POST    = 3,
    /* Server -> Client */
    MSG_BOARD   = 4,
    MSG_STATUS  = 5,
} MsgType;

typedef enum MsgStatus {
    MSG_STATUS_SUCCESS = 0,
} MsgStatus;

/**
 * Data type for messages exchanged between client/server.
 */
typedef struct MsgHdr {
    short type;
    short status;
    int   dataSize;
    char  data[0];
} MsgHdr;


void Log(const char *fmt, ...);
void Error(const char *fmt, ...);

int ReadFully(int sd, void *buf, int nbytes);
int WriteFully(int sd, void *buf, int nbytes);

void SocketAddrToString(const struct sockaddr_in *addr, char *addrStr,
                        int addrStrLen);
void SocketAddrToString6(const struct sockaddr *addr, char *addrStr,
                         int addrStrLen);
void PrintMsg(const MsgHdr *msg, const char *prefix);

#endif
