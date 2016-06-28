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
#ifndef ccnxTestrig_PacketUtility_h
#define ccnxTestrig_PacketUtility_h

#include "ccnxTestrig_SuiteTestResult.h"

#include <ccnx/transport/common/transport_MetaMessage.h>
#include <ccnx/transport/common/transport_Message.h>

typedef enum {
    CCNxInterestFieldError_Name,
    CCNxInterestFieldError_Lifetime,
    CCNxInterestFieldError_HopLimit,
    CCNxInterestFieldError_KeyIdRestriction,
    CCNxInterestFieldError_ContentObjectHashRestriction,
    CCNxInterestFieldError_None
} CCNxInterestFieldError;

typedef enum {
    CCNxContentObjectFieldError_Name,
    CCNxContentObjectFieldError_Payload,
    CCNxContentObjectFieldError_None
} CCNxContentObjectFieldError;

typedef enum {
    CCNxManifestFieldError_Name,
    CCNxManifestFieldError_Payload,
    CCNxManifestFieldError_None
} CCNxManifestFieldError;

/**
 * Determine if the "packet pair" is valid. The sent packet will be that
 * which was sent to the forwarder and the received packet is that which was
 * received from the forwarder.
 *
 * The forwarder is expected to make some modifications to the packet, e.g., by
 * decrementing the hop count. This function checks those conditions.
 *
 * @param [in] sent The `CCNxTlvDictionary` sent packet
 * @param [in] received The `CCNxTlvDictionary` received packet
 * @param [in] result The `CCNxTestrigSuiteTestResult` in which to record any errors.
 *
 * @return The possibly modified `CCNxTestrigSuiteTestResult` instance.
 *
 * Example:
 * @code
 * {
 *     CCNxTlvDictionary *sent = ...
 *     CCNxTlvDictionary *received = ...
 *     CCNxTestrigSuiteTestResult *result = ...
 *
 *     result = ccnxTestrigPacketUtility_IsValidPacketPair(sent, received, result);
 * }
 * @endcode
 */
CCNxTestrigSuiteTestResult *ccnxTestrigPacketUtility_IsValidPacketPair(CCNxTlvDictionary *sent,
    CCNxMetaMessage *received, CCNxTestrigSuiteTestResult *result);

/**
 * Encode a packet to its wire format.
 *
 * @param [in] packetDictionary The `CCNxTlvDictionary` to encode.
 *
 * @return A `PARCBuffer` containing the wire-encoded packet.
 *
 * Example:
 * @code
 * {
 *     CCNxTlvDictionary *message = ...
 *
 *     PARCBuffer *wireEncodedFormat = ccnxTestrigPacketUtility_EncodePacket(message);
 * }
 * @endcode
 */
PARCBuffer *ccnxTestrigPacketUtility_EncodePacket(CCNxTlvDictionary *packetDictionary);
#endif // ccnxTestrig_PacketUtility_h
