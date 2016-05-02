#ifndef ccnxTestrigLink_h_
#define ccnxTestrigLink_h_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <parc/algol/parc_Buffer.h>

struct ccnx_testrig_link;
typedef struct ccnx_testrig_link CCNxTestrigLink;

typedef enum {
    CCNxTestrigLinkType_UDP = 0,
    CCNxTestrigLinkType_TCP = 1,
    CCNxTestrigLinkType_ETH = 2,
    CCNxTestrigLinkType_Invalid = 3
} CCNxTestrigLinkType;

CCNxTestrigLink *ccnxTestrigLink_Acquire(const CCNxTestrigLink *link);
void ccnxTestrigLink_Release(CCNxTestrigLink **linkPtr);

CCNxTestrigLink *ccnxTestrigLink_Listen(CCNxTestrigLinkType type, char *address, int port);
CCNxTestrigLink *ccnxTestrigLink_Connect(CCNxTestrigLinkType type, char *address, int port);

void ccnxTestrigLink_SetTimeout(CCNxTestrigLink *link, struct timeval tv);

PARCBuffer *ccnxTestrigLink_Receive(CCNxTestrigLink *link);
int ccnxTestrigLink_Send(CCNxTestrigLink *link, PARCBuffer *buffer);

void ccnxTestrigLink_Close(CCNxTestrigLink *link);


#endif // ccnxTestrigLink_h_
