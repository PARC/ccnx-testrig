#ifndef ccnxTestrigLink_h_
#define ccnxTestrigLink_h_

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

struct ccnx_testrig_link;
typedef struct ccnx_testrig_link CCNxTestrigLink;

typedef enum {
    CCNxTestrigLinkType_UDP = 0,
    CCNxTestrigLinkType_TCP = 1,
    CCNxTestrigLinkType_ETH = 2,
    CCNxTestrigLinkType_Invalid = 3
} CCNxTestrigLinkType;

CCNxTestrigLink *ccnxTestrigLink_Acquire(CCNxTestrigLink *link);
void ccnxTestrigLink_Release(CCNxTestrigLink **linkPtr);

CCNxTestrigLink *ccnxTestrigLink_Listen(CCNxTestrigLinkType type, char *address, int port);
CCNxTestrigLink *ccnxTestrigLink_Connect(CCNxTestrigLinkType type, char *address, int port);

void ccnxTestrigLink_SetTimeout(CCNxTestrigLink *link, struct timeval tv);

int ccnxTestrigLink_Receive(CCNxTestrigLink *link, uint8_t *buffer);
int ccnxTestrigLink_Send(CCNxTestrigLink *link, uint8_t *buffer, int length);

void ccnxTestrigLink_Close(CCNxTestrigLink *link);


#endif // ccnxTestrigLink_h_
