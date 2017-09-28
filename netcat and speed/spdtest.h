#ifndef _SPDTEST_H_
#define _SPDTEST_H_

#define  SPDTEST_NUM_ITERS    30

typedef struct SpdTestHdr {
    unsigned int seq;
} SpdTestHdr;

typedef struct SpdTestMsg {
    SpdTestHdr   hdr;
    char         data[1];
} SpdTestMsg;

typedef SpdTestHdr SpdTestAck;

#endif
