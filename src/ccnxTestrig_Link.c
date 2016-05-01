#include "ccnxTestrig_link.h"
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

#include "util.h"

struct ccnx_testrig_link {
    CCNxTestrigLinkType type;

    int (*receiveFunction)(CCNxTestrigLink *, uint8_t *);
    int (*sendFunction)(CCNxTestrigLink *, uint8_t *, int);

    int port;
    int socket;
    struct sockaddr_in sourceAddress;
    char *hostAddress;

    int targetSocket;
    struct sockaddr_in targetAddress;
    unsigned int targetAddressLength;
};

static bool
_ccnxTestrigLink_Destructor(CCNxTestrigLink **linkPtr)
{
    CCNxTestrigLink *link = *linkPtr;
    // TODO
    return true;
}

parcObject_ImplementAcquire(ccnxTestrigLink, CCNxTestrigLink);
parcObject_ImplementRelease(ccnxTestrigLink, CCNxTestrigLink);

parcObject_Override(
	CCNxTestrigLink, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigLink_Destructor);

static int
_udp_receive(CCNxTestrigLink *link, uint8_t *buffer)
{
    int numBytesReceived = recvfrom(link->socket, buffer, MTU, 0, (struct sockaddr *) &(link->targetAddress), &(link->targetAddressLength));
    if (numBytesReceived < 0) {
        fprintf(stderr, "recvfrom() failed");
    }
    return numBytesReceived;
}

static int
_udp_send(CCNxTestrigLink *link, uint8_t *buffer, int length)
{
    int val = sendto(link->socket, buffer, length, 0,
        (struct sockaddr *) &link->targetAddress, link->targetAddressLength);
    return val;
}

static int
_tcp_receive(CCNxTestrigLink *link, uint8_t *buffer)
{
    int recvMsgSize = recv(link->targetSocket, buffer, MTU, 0);
    return recvMsgSize;
}

static int
_tcp_send(CCNxTestrigLink *link, uint8_t *buffer, int length)
{
    int numSent = send(link->targetSocket, buffer, length, 0);
    return numSent;
}

static CCNxTestrigLink *
_create_link()
{
    CCNxTestrigLink *link = parcObject_CreateInstance(CCNxTestrigLink);

    if (link != NULL) {
        link->port = 0;
        link->socket = 0;
        link->hostAddress = NULL;
    }

    return link;
}

static CCNxTestrigLink *
_create_udp_ccnxTestrigLink_listener(char *address, int port)
{
    CCNxTestrigLink *link = _create_link();

    link->type = CCNxTestrigLinkType_UDP;
    link->port = port;
    link->receiveFunction = _udp_receive;
    link->sendFunction = _udp_send;
    link->targetAddressLength = sizeof(link->targetAddress);

    if ((link->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "socket() failed");
    }

    // set address reuse
    int optval = 1;
    setsockopt(link->socket, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval , sizeof(int));

    // initialize the address
    bzero((char *) &link->sourceAddress, sizeof(link->sourceAddress));
    link->sourceAddress.sin_family = AF_INET;
    link->sourceAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    link->sourceAddress.sin_port = htons(link->port);

    if (bind(link->socket, (struct sockaddr *) &(link->sourceAddress), sizeof(link->sourceAddress)) < 0) {
        fprintf(stderr, "bind() failed");
    }

    return link;
}

static CCNxTestrigLink *
_create_tcp_ccnxTestrigLink_listener(char *address, int port)
{
    CCNxTestrigLink *link = _create_link();

    link->type = CCNxTestrigLinkType_TCP;
    link->port = port;
    link->targetAddressLength = sizeof(link->targetAddress);
    link->receiveFunction = _tcp_receive;
    link->sendFunction = _tcp_send;

    if ((link->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "socket() failed");
    }

    memset(&(link->sourceAddress), 0, sizeof(link->sourceAddress));
    link->sourceAddress.sin_family = AF_INET;
    link->sourceAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    link->sourceAddress.sin_port = htons(link->port);

    int on = 1;
    if (setsockopt(link->socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0) {
        fprintf(stderr, "setsockopt() failed");
    }

    if (bind(link->socket, (struct sockaddr *) &(link->sourceAddress), sizeof(link->sourceAddress)) < 0) {
        fprintf(stderr, "bind() failed");
    }

    if (listen(link->socket, MAX_NUMBER_OF_TCP_CONNECTIONS) < 0) {
        fprintf(stderr, "listen() failed");
    }

    if ((link->targetSocket = accept(link->socket, (struct sockaddr *) &(link->targetAddress), &link->targetAddressLength)) < 0) {
        fprintf(stderr, "accept() failed");
    }

    return link;
}

static Link *
_create_udp_link(char *address, int port)
{
    CCNxTestrigLink *link = _create_link();

    link->type = CCNxTestrigLinkType_UDP;
    link->port = port;
    link->targetAddressLength = sizeof(link->targetAddress);
    link->receiveFunction = _udp_receive;
    link->sendFunction = _udp_send;

    if ((link->socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "socket() failed");
    }

    memset(&(link->targetAddress), 0, sizeof(link->targetAddress));
    link->targetAddress.sin_family = AF_INET;
    link->targetAddress.sin_addr.s_addr = inet_addr(address);
    link->targetAddress.sin_port = htons(link->port);

    return link;
}

static CCNxTestrigLink *
_create_tcp_link(char *address, int port)
{
    CCNxTestrigLink *link = _create_link();

    link->type = CCNxTestrigLinkType_TCP;
    link->port = port;
    link->targetAddressLength = sizeof(link->targetAddress);
    link->receiveFunction = _tcp_receive;
    link->sendFunction = _tcp_send;

    if ((link->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "socket() failed");
    }
    link->targetSocket = link->socket;

    memset(&(link->targetAddress), 0, sizeof(link->targetAddress));
    link->targetAddress.sin_family = AF_INET;
    link->targetAddress.sin_addr.s_addr = inet_addr(address);
    link->targetAddress.sin_port = htons(link->port);

    int on = 1;
    if (setsockopt(link->socket, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)) < 0) {
        fprintf(stderr, "setsockopt() failed");
    }

    if (connect(link->socket, (struct sockaddr *) &link->targetAddress, sizeof(link->targetAddress)) < 0) {
        fprintf(stderr, "connect() failed");
    }

    return link;
}

CCNxTestrigLink *
ccnxTestrigLink_Listen(CCNxTestrigLinkType type, char *address, int port)
{
    switch (type) {
        case LinkType_UDP:
            return _create_udp_ccnxTestrigLink_listener(address, port);
        case LinkType_TCP:
            return _create_tcp_ccnxTestrigLink_listener(address, port);
        default:
            fprintf(stderr, "Error: invalid LinkType specified: %d", type);
            return NULL;
    }
}

CCNxTestrigLink *
ccnxTestrigLink_Connect(CCNxTestrigLinkType type, char *address, int port)
{
    switch (type) {
        case LinkType_UDP:
            return _create_udp_link(address, port);
        case LinkType_TCP:
            return _create_tcp_link(address, port);
        default:
            fprintf(stderr, "Error: invalid LinkType specified: %d", type);
            return NULL;
    }
}

void
ccnxTestrigLink_SetTimeout(CCNxTestrigLink *link, struct timeval tv)
{
    setsockopt(link->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
}

int
ccnxTestrigLink_Receive(CCNxTestrigLink *link, uint8_t *buffer)
{
    return link->receiveFunction(link, buffer);
}

int
ccnxTestrigLink_Send(CCNxTestrigLink *link, uint8_t *buffer, int length)
{
    return link->sendFunction(link, buffer, length);
}

void
ccnxTestrigLink_Close(CCNxTestrigLink *link)
{
    close(link->socket);
    if (link->socket != link->targetSocket) {
        close(link->targetSocket);
    }
}
