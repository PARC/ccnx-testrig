/*
 * Copyright (c) 2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ################################################################################
 * #
 * # PATENT NOTICE
 * #
 * # This software is distributed under the BSD 2-clause License (see LICENSE
 * # file).  This BSD License does not make any patent claims and as such, does
 * # not act as a patent grant.  The purpose of this section is for each contributor
 * # to define their intentions with respect to intellectual property.
 * #
 * # Each contributor to this source code is encouraged to state their patent
 * # claims and licensing mechanisms for any contributions made. At the end of
 * # this section contributors may each make their own statements.  Contributor's
 * # claims and grants only apply to the pieces (source code, programs, text,
 * # media, etc) that they have contributed directly to this software.
 * #
 * # There is no guarantee that this section is complete, up to date or accurate. It
 * # is up to the contributors to maintain their portion of this section and up to
 * # the user of the software to verify any claims herein.
 * #
 * # Do not remove this header notification.  The contents of this section must be
 * # present in all distributions of the software.  You may only modify your own
 * # intellectual property statements.  Please provide contact information.
 *
 * - Palo Alto Research Center, Inc
 * This software distribution does not grant any rights to patents owned by Palo
 * Alto Research Center, Inc (PARC). Rights to these patents are available via
 * various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
 * intellectual property used by its contributions to this software. You may
 * contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
 */
 #include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <poll.h>

#include <parc/algol/parc_Object.h>

#include "ccnxTestrig_link.h"

#define MTU 4096
#define MAX_NUMBER_OF_TCP_CONNECTIONS 3

struct ccnx_testrig_link {
    CCNxTestrigLinkType type;

    PARCBuffer *(*receiveFunction)(CCNxTestrigLink *, int);
    int (*sendFunction)(CCNxTestrigLink *, PARCBuffer *);

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

static PARCBuffer *
_udp_receive(CCNxTestrigLink *link, int timeout)
{
    struct pollfd fd;
    fd.fd = link->socket;
    fd.events = POLLIN;
    int res = poll(&fd, 1, timeout);

    if (res == 0) {
        return NULL;
    } else if (res == -1) {
        perror("An error occurred while receiving");
        return NULL;
    } else {
        uint8_t buffer[MTU];
        int numBytesReceived = recvfrom(link->socket, buffer, MTU, 0, (struct sockaddr *) &(link->targetAddress), &(link->targetAddressLength));
        if (numBytesReceived < 0) {
            fprintf(stderr, "recvfrom() failed");
            return NULL;
        }

        PARCBuffer *result = parcBuffer_Allocate(numBytesReceived);
        parcBuffer_PutArray(result, numBytesReceived, buffer);
        parcBuffer_Flip(result);

        return result;
    }
}

static int
_udp_send(CCNxTestrigLink *link, PARCBuffer *buffer)
{
    size_t length = parcBuffer_Remaining(buffer);
    uint8_t *bufferOverlay = parcBuffer_Overlay(buffer, length);
    int val = sendto(link->socket, bufferOverlay, length, 0,
        (struct sockaddr *) &link->targetAddress, link->targetAddressLength);

    printf("sent %d bytes\n", val);
    return val;
}

static PARCBuffer *
_tcp_receive(CCNxTestrigLink *link, int timeout)
{
    struct pollfd fd;
    fd.fd = link->targetSocket;
    fd.events = POLLIN;
    int res = poll(&fd, 1, timeout);

    if (res == 0) {
        return NULL;
    } else if (res == -1) {
        perror("An error occurred while receiving");
        return NULL;
    } else {
        uint8_t buffer[MTU];
        int recvMsgSize = recv(link->targetSocket, buffer, MTU, 0);

        PARCBuffer *result = parcBuffer_Allocate(recvMsgSize);
        parcBuffer_PutArray(result, recvMsgSize, buffer);
        parcBuffer_Flip(result);

        return result;
    }
}

static int
_tcp_send(CCNxTestrigLink *link, PARCBuffer *buffer)
{
    size_t length = parcBuffer_Remaining(buffer);
    uint8_t *bufferOverlay = parcBuffer_Overlay(buffer, length);
    int numSent = send(link->targetSocket, bufferOverlay, length, 0);
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

    printf("Accepted!\n");

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

    printf("Accepted!\n");

    return link;
}

static CCNxTestrigLink *
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
        case CCNxTestrigLinkType_UDP:
            return _create_udp_ccnxTestrigLink_listener(address, port);
        case CCNxTestrigLinkType_TCP:
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
        case CCNxTestrigLinkType_UDP:
            return _create_udp_link(address, port);
        case CCNxTestrigLinkType_TCP:
            return _create_tcp_link(address, port);
        default:
            fprintf(stderr, "Error: invalid LinkType specified: %d", type);
            return NULL;
    }
}

PARCBuffer *
ccnxTestrigLink_Receive(CCNxTestrigLink *link)
{
    return link->receiveFunction(link, -1);
}

PARCBuffer *
ccnxTestrigLink_ReceiveWithTimeout(CCNxTestrigLink *link, int timeout)
{
    return link->receiveFunction(link, timeout);
}

int
ccnxTestrigLink_Send(CCNxTestrigLink *link, PARCBuffer *buffer)
{
    return link->sendFunction(link, buffer);
}

void
ccnxTestrigLink_Close(CCNxTestrigLink *link)
{
    close(link->socket);
    if (link->socket != link->targetSocket) {
        close(link->targetSocket);
    }
}
