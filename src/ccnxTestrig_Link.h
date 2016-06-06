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
    CCNxTestrigLinkType_Invalid = 3
} CCNxTestrigLinkType;

CCNxTestrigLink *ccnxTestrigLink_Acquire(const CCNxTestrigLink *link);
void ccnxTestrigLink_Release(CCNxTestrigLink **linkPtr);

CCNxTestrigLink *ccnxTestrigLink_Listen(CCNxTestrigLinkType type, char *address, int port);
CCNxTestrigLink *ccnxTestrigLink_Connect(CCNxTestrigLinkType type, char *address, int port);

PARCBuffer *ccnxTestrigLink_Receive(CCNxTestrigLink *link);
PARCBuffer *ccnxTestrigLink_ReceiveWithTimeout(CCNxTestrigLink *link, int timeout);
int ccnxTestrigLink_Send(CCNxTestrigLink *link, PARCBuffer *buffer);

void ccnxTestrigLink_Close(CCNxTestrigLink *link);


#endif // ccnxTestrigLink_h_
