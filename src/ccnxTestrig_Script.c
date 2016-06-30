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
#include "ccnxTestrig_SuiteTestResult.h"
#include "ccnxTestrig_Script.h"
#include "ccnxTestrig_PacketUtility.h"

#include <parc/algol/parc_LinkedList.h>

#include <ccnx/common/ccnx_Name.h>
#include <ccnx/common/ccnx_Interest.h>
#include <ccnx/common/ccnx_ContentObject.h>
#include <ccnx/common/ccnx_Manifest.h>

#include <ccnx/transport/common/transport_MetaMessage.h>
#include <ccnx/transport/common/transport_Message.h>

struct ccnx_testrig_script {
    char *testCase;
    PARCLinkedList *steps;
};

static bool
_ccnxTestrigScript_Destructor(CCNxTestrigScript **resultPtr)
{
    CCNxTestrigScript *result = *resultPtr;

    parcMemory_Deallocate(&result->testCase);
    parcLinkedList_Release(&result->steps);

    return true;
}

parcObject_ImplementAcquire(ccnxTestrigScript, CCNxTestrigScript);
parcObject_ImplementRelease(ccnxTestrigScript, CCNxTestrigScript);

parcObject_Override(
	CCNxTestrigScript, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigScript_Destructor);

struct ccnx_testrig_script_step {
    int stepIndex;
    CCNxTlvDictionary *packet;

    // Step link information
    PARCBitVector *linkVector;

    // Execute function pointer
    CCNxTestrigSuiteTestResult *(*execute)(CCNxTestrigScriptStep *, CCNxTestrigSuiteTestResult *, CCNxTestrig *);

    // Execute parameters
    CCNxTestrigScriptStep *reference;

    // Result parameters
    PARCBitVector *receivedLinkVector;
};

static bool
_ccnxTestrigScriptStep_Destructor(CCNxTestrigScriptStep **resultPtr)
{
    CCNxTestrigScriptStep *step = *resultPtr;
    parcBitVector_Release(&step->linkVector);
    return true;
}

parcObject_ImplementAcquire(ccnxTestrigScriptStep, CCNxTestrigScriptStep);
parcObject_ImplementRelease(ccnxTestrigScriptStep, CCNxTestrigScriptStep);

parcObject_Override(
	CCNxTestrigScriptStep, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigScriptStep_Destructor);

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteSendStep(CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    PARCBuffer *packetBuffer = ccnxTestrigPacketUtility_EncodePacket(step->packet);
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkByID(rig, parcBitVector_NextBitSet(step->linkVector, 0)), packetBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(result, packetBuffer);
    return result;
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteReceiveAllStep(CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    bool succeeded = true;
    int currentBit = 0;

    step->receivedLinkVector = parcBitVector_Create();

    for (size_t i = 0; i < parcBitVector_NumberOfBitsSet(step->linkVector); i++) {
        currentBit = parcBitVector_NextBitSet(step->linkVector, currentBit);

        PARCBuffer *receiveBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkByID(rig, currentBit), 1000);
        if (receiveBuffer == NULL) {
            ccnxTestrigSuiteTestResult_SetFail(result, "Failed to receive a message in the allotted time.");
            return result;
        }

        parcBitVector_Set(step->receivedLinkVector, currentBit);

        CCNxTlvDictionary *referencedMessage = (step->reference)->packet;
        CCNxMetaMessage *reconstructedMessage = ccnxMetaMessage_CreateFromWireFormatBuffer(receiveBuffer);

        // Check that the message types are equal
        if (!(ccnxMetaMessage_IsInterest(reconstructedMessage) == ccnxTlvDictionary_IsInterest(referencedMessage))) {
            ccnxTestrigSuiteTestResult_SetFail(result, "The received message type does not match the sent message type (INTEREST)");
            return result;
        }
        if (!(ccnxMetaMessage_IsContentObject(reconstructedMessage) == ccnxTlvDictionary_IsContentObject(referencedMessage))) {
            ccnxTestrigSuiteTestResult_SetFail(result, "The received message type does not match the sent message type (CONTENT)");
            return result;
        }
        if (!(ccnxMetaMessage_IsManifest(reconstructedMessage) == ccnxTlvDictionary_IsManifest(referencedMessage))) {
            ccnxTestrigSuiteTestResult_SetFail(result, "The received message type does not match the sent message type (MANIFEST)");
            return result;
        }

        result = ccnxTestrigPacketUtility_IsValidPacketPair(referencedMessage, reconstructedMessage, result);
    }

    return result;
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteReceiveOneStep(CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    bool succeeded = false;
    int currentBit = 0;
    step->receivedLinkVector = parcBitVector_Create();
    for (size_t i = 0; i < parcBitVector_NumberOfBitsSet(step->linkVector); i++) {
        currentBit = parcBitVector_NextBitSet(step->linkVector, currentBit);

        PARCBuffer *receiveBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkByID(rig, currentBit), 1000);
        if (receiveBuffer == NULL) {
            continue;
        }

        parcBitVector_Set(step->receivedLinkVector, currentBit);

        CCNxTlvDictionary *referencedMessage = (step->reference)->packet;
        CCNxMetaMessage *reconstructedMessage = ccnxMetaMessage_CreateFromWireFormatBuffer(receiveBuffer);

        // Check that the message types are equal
        if (!(ccnxMetaMessage_IsInterest(reconstructedMessage) == ccnxTlvDictionary_IsInterest(referencedMessage))) {
            ccnxTestrigSuiteTestResult_SetFail(result, "The received message type does not match the sent message type (INTEREST)");
            return result;
        }
        if (!(ccnxMetaMessage_IsContentObject(reconstructedMessage) == ccnxTlvDictionary_IsContentObject(referencedMessage))) {
            ccnxTestrigSuiteTestResult_SetFail(result, "The received message type does not match the sent message type (CONTENT)");
            return result;
        }
        if (!(ccnxMetaMessage_IsManifest(reconstructedMessage) == ccnxTlvDictionary_IsManifest(referencedMessage))) {
            ccnxTestrigSuiteTestResult_SetFail(result, "The received message type does not match the sent message type (MANIFEST)");
            return result;
        }

        result = ccnxTestrigPacketUtility_IsValidPacketPair(referencedMessage, reconstructedMessage, result);
    }

    if (!succeeded) {
        ccnxTestrigSuiteTestResult_SetFail(result, "Did not receive any message on the specified links.");
    }

    return result;
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteReceiveNoneStep(CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    bool succeeded = false;
    int currentBit = 0;
    step->receivedLinkVector = parcBitVector_Create();
    for (size_t i = 0; i < parcBitVector_NumberOfBitsSet(step->linkVector); i++) {
        currentBit = parcBitVector_NextBitSet(step->linkVector, currentBit);

        PARCBuffer *receiveBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkByID(rig, currentBit), 1000);
        if (receiveBuffer != NULL) {
            parcBitVector_Set(step->receivedLinkVector, currentBit);
            ccnxTestrigSuiteTestResult_SetFail(result, "Received a message when we expected not to.");
            break;
        }
    }
    return result;
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteStep(CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    return step->execute(step, result, rig);
}

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateSendStep(int index, CCNxTestrigLinkID linkId, CCNxTlvDictionary *messageDictionary)
{
    CCNxTestrigScriptStep *step = parcObject_CreateInstance(CCNxTestrigScriptStep);
    if (step != NULL) {
        step->stepIndex = index;
        step->packet = ccnxTlvDictionary_Acquire(messageDictionary);
        step->reference = NULL;
        step->execute = _ccnxTestrigScript_ExecuteSendStep;

        step->linkVector = parcBitVector_Create();
        parcBitVector_Set(step->linkVector, linkId);
    }
    return step;
}

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateRespondStep(int index, CCNxTestrigScriptStep *reference, CCNxTlvDictionary *packet)
{
    CCNxTestrigScriptStep *step = parcObject_CreateInstance(CCNxTestrigScriptStep);
    if (step != NULL) {
        step->stepIndex = index;
        step->packet = ccnxTlvDictionary_Acquire(packet);
        step->reference = NULL;
        step->execute = _ccnxTestrigScript_ExecuteSendStep;
        step->linkVector = parcBitVector_Acquire(reference->receivedLinkVector);
    }
    return step;
}

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateReceive(int index, CCNxTestrigScriptStep *reference, PARCBitVector *linkVector)
{
    CCNxTestrigScriptStep *step = parcObject_CreateInstance(CCNxTestrigScriptStep);
    if (step != NULL) {
        step->stepIndex = index;
        step->packet = NULL;
        step->reference = ccnxTestrigScriptStep_Acquire(reference);

        step->linkVector = parcBitVector_Acquire(linkVector);
    }
    return step;
}

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateReceiveAllStep(int index, CCNxTestrigScriptStep *reference, PARCBitVector *linkVector)
{
    CCNxTestrigScriptStep *step = _ccnxTestrigScriptStep_CreateReceive(index, reference, linkVector);
    if (step != NULL) {
        step->execute = _ccnxTestrigScript_ExecuteReceiveAllStep;
    }
    return step;
}

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateReceiveNoneStep(int index, CCNxTestrigScriptStep *reference, PARCBitVector *linkVector)
{
    CCNxTestrigScriptStep *step = _ccnxTestrigScriptStep_CreateReceive(index, reference, linkVector);
    if (step != NULL) {
        step->execute = _ccnxTestrigScript_ExecuteReceiveNoneStep;
    }
    return step;
}

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateReceiveOneStep(int index, CCNxTestrigScriptStep *reference, PARCBitVector *linkVector)
{
    CCNxTestrigScriptStep *step = _ccnxTestrigScriptStep_CreateReceive(index, reference, linkVector);
    if (step != NULL) {
        step->execute = _ccnxTestrigScript_ExecuteReceiveOneStep;
    }
    return step;
}

CCNxTestrigScript *
ccnxTestrigScript_Create(char *testCase)
{
    CCNxTestrigScript *result = parcObject_CreateInstance(CCNxTestrigScript);

    if (result != NULL) {
        result->testCase = malloc(strlen(testCase));
        strcpy(result->testCase, testCase);
        result->steps = parcLinkedList_Create();
    }

    return result;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddSendStep(CCNxTestrigScript *script, CCNxTlvDictionary *messageDictionary, CCNxTestrigLinkID linkId)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *step = _ccnxTestrigScriptStep_CreateSendStep(index, linkId, messageDictionary);
    parcLinkedList_Append(script->steps, step);
    return step;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddRespondStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, CCNxTlvDictionary *packet)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *newStep = _ccnxTestrigScriptStep_CreateRespondStep(index, step, packet);
    parcLinkedList_Append(script->steps, newStep);
    return newStep;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddReceiveOneStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, PARCBitVector *linkVector)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *newStep = _ccnxTestrigScriptStep_CreateReceiveOneStep(index, step, linkVector);
    parcLinkedList_Append(script->steps, newStep);
    return newStep;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddReceiveNoneStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, PARCBitVector *linkVector)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *newStep = _ccnxTestrigScriptStep_CreateReceiveNoneStep(index, step, linkVector);
    parcLinkedList_Append(script->steps, newStep);
    return newStep;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddReceiveAllStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, PARCBitVector *linkVector)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *newStep = _ccnxTestrigScriptStep_CreateReceiveAllStep(index, step, linkVector);
    parcLinkedList_Append(script->steps, newStep);
    return newStep;
}

CCNxTestrigSuiteTestResult *
ccnxTestrigScript_Execute(CCNxTestrigScript *script, CCNxTestrig *rig)
{
    int numSteps = parcLinkedList_Size(script->steps);
    CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create(script->testCase);

    for (int i = 0; i < numSteps; i++) {
        printf(">> Executing step %d\n", i);
        CCNxTestrigScriptStep *step = parcLinkedList_GetAtIndex(script->steps, i);
        result = _ccnxTestrigScript_ExecuteStep(step, result, rig);

        // If the last step failed, stop the test and return the failure.
        if (ccnxTestrigSuiteTestResult_IsFailure(result)) {
            printf(">> **** Failed at step %d\n", i);
            return result;
        }
    }

    return ccnxTestrigSuiteTestResult_SetPass(result);
}
