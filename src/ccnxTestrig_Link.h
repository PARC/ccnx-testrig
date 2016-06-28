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

/**
 * The set of available link types.
 */
typedef enum {
    CCNxTestrigLinkType_UDP = 0,
    CCNxTestrigLinkType_TCP = 1,
    CCNxTestrigLinkType_Invalid = 3
} CCNxTestrigLinkType;

/**
 * Increase the number of references to a `CCNxTestrigLink` instance.
 *
 * Note that new `CCNxTestrigLink` is not created,
 * only that the given `CCNxTestrigLink` reference count is incremented.
 * Discard the reference by invoking `ccnxTestrigLink_Release`.
 *
 * @param [in] instance A pointer to a valid CCNxTestrigLink instance.
 *
 * @return The same value as @p instance.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *a = ccnxTestrigLink_Listen(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *
 *     CCNxTestrigLink *b = ccnxManifestBuilder_Acquire(a);
 *
 *     ccnxTestrigLink_Release(&a);
 *     ccnxTestrigLink_Release(&b);
 * }
 * @endcode
 */
CCNxTestrigLink *ccnxTestrigLink_Acquire(const CCNxTestrigLink *link);

/**
 * Release a previously acquired reference to the given `CCNxTestrigLink` instance,
 * decrementing the reference count for the instance.
 *
 * The pointer to the instance is set to NULL as a side-effect of this function.
 *
 * If the invocation causes the last reference to the instance to be released,
 * the instance is deallocated and the instance's implementation will perform
 * additional cleanup and release other privately held references.
 *
 * @param [in,out] instancePtr A pointer to a pointer to the instance to release.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *link = ccnxTestrigLink_Listen(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *
 *     ccnxTestrigLink_Release(&link);
 * }
 * @endcode
 */
void ccnxTestrigLink_Release(CCNxTestrigLink **linkPtr);

/**
 * Create a new link by listening at the specified address and port.
 * The `type` parameter determines the link protocol to be used.
 *
 * @param [in] type The type of link.
 * @param [in] address The address of the link.
 * @param [in] port The port of the link.
 *
 * @return A newly allocated `CCNxTestrigLink` that must be freed by `ccnxTestrigLink_Release`.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *link = ccnxTestrigLink_Listen(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *
 *     ccnxTestrigLink_Release(&link);
 * }
 * @endcode
 */
CCNxTestrigLink *ccnxTestrigLink_Listen(CCNxTestrigLinkType type, char *address, int port);

/**
 * Create a new link by connecting to another entity at the address and port given.
 * The `type` parameter determines the link protocol to be used.
 *
 * @param [in] type The type of link.
 * @param [in] address The address of the link.
 * @param [in] port The port of the link.
 *
 * @return A newly allocated `CCNxTestrigLink` that must be freed by `ccnxTestrigLink_Release`.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *link = ccnxTestrigLink_Connect(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *
 *     ccnxTestrigLink_Release(&link);
 * }
 * @endcode
 */
CCNxTestrigLink *ccnxTestrigLink_Connect(CCNxTestrigLinkType type, char *address, int port);

/**
 * Receive a packet from the specified `CCNxTestrigLink`.
 *
 * This function blocks until a packet is read.
 *
 * @param [in] link The link from which to receive a packet.
 *
 * @return A `PARCBuffer` containing the packet read from the link.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *link = ccnxTestrigLink_Connect(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *
 *     PARCBuffer *packet = ccnxTestrigLink_Receive(link);
 *
 *     ccnxTestrigLink_Release(&link);
 * }
 * @endcode
 */
PARCBuffer *ccnxTestrigLink_Receive(CCNxTestrigLink *link);

/**
 * Receive a packet from the specified `CCNxTestrigLink`.
 *
 * This function times out after the given number of seconds.
 *
 * @param [in] link The link from which to receive a packet.
 * @param [in] timeout The number of seconds to wait for a packet.
 *
 * @retval A `PARCBuffer` containing the packet read from the link.
 * @retval NULL if the read function timed out.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *link = ccnxTestrigLink_Connect(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *
 *     PARCBuffer *packet = ccnxTestrigLink_ReceiveWithTimeout(link, 1);
 *
 *     ccnxTestrigLink_Release(&link);
 * }
 * @endcode
 */
PARCBuffer *ccnxTestrigLink_ReceiveWithTimeout(CCNxTestrigLink *link, int timeout);

/**
 * Send a packet on the specified `CCNxTestrigLink`.
 *
 * @param [in] link The link from which to receive a packet.
 * @param [in] buffer The wire-encoded packet to send on the link.
 *
 * @return The number of bytes written.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *link = ccnxTestrigLink_Connect(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *     PARCBuffer *packet = ...
 *
 *     ccnxTestrigLink_Send(link, packet);
 *
 *     ccnxTestrigLink_Release(&link);
 * }
 * @endcode
 */
int ccnxTestrigLink_Send(CCNxTestrigLink *link, PARCBuffer *buffer);

/**
 * Close the specified `CCNxTestrigLink`.
 *
 * @param [in] link The `CCNxTestrigLink` to close.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigLink *link = ccnxTestrigLink_Connect(CCNxTestrigLinkType_UDP, "localhost", 9696);
 *
 *     ccnxTestrigLink_Close(link);
 *     ccnxTestrigLink_Release(&link);
 * }
 * @endcode
 */
void ccnxTestrigLink_Close(CCNxTestrigLink *link);

#endif // ccnxTestrigLink_h_
