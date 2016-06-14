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

    parcMemory_Deallocate(&(result->testCase));

    return true;
}

parcObject_ImplementAcquire(ccnxTestrigScript, CCNxTestrigScript);
parcObject_ImplementRelease(ccnxTestrigScript, CCNxTestrigScript);

parcObject_Override(
	CCNxTestrigScript, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigScript_Destructor);

struct ccnx_testrig_script_step {
    int stepIndex;
    CCNxTestrigLinkID linkId;
    CCNxTlvDictionary *packet;
    bool send;

    // Receive step conditions
    // XX fix this.
    CCNxTestrigScriptStep *reference;
    bool nullCheck;
    char *failureMessage;
};

static bool
_ccnxTestrigScriptStep_Destructor(CCNxTestrigScriptStep **resultPtr)
{
    return true;
}

parcObject_ImplementAcquire(ccnxTestrigScriptStep, CCNxTestrigScriptStep);
parcObject_ImplementRelease(ccnxTestrigScriptStep, CCNxTestrigScriptStep);

parcObject_Override(
	CCNxTestrigScriptStep, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigScriptStep_Destructor);

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateSendStep(int index, CCNxTestrigLinkID linkId, CCNxTlvDictionary *messageDictionary)
{
    CCNxTestrigScriptStep *step = parcObject_CreateInstance(CCNxTestrigScriptStep);
    if (step != NULL) {
        step->stepIndex = index;
        step->linkId = linkId;
        step->packet = ccnxTlvDictionary_Acquire(messageDictionary);
        step->send = true;
        step->reference = NULL;
        step->failureMessage = NULL;
        step->nullCheck = false;
    }
    return step;
}

static CCNxTestrigScriptStep *
_ccnxTestrigScriptStep_CreateReceiveStep(int index, CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *reference, bool nullCheck, char *failureMessage)
{
    CCNxTestrigScriptStep *step = parcObject_CreateInstance(CCNxTestrigScriptStep);
    if (step != NULL) {
        step->stepIndex = index;
        step->linkId = linkId;
        step->packet = NULL;
        step->send = false;
        step->reference = ccnxTestrigScriptStep_Acquire(reference);

        step->failureMessage = parcMemory_StringDuplicate(failureMessage, strlen(failureMessage));
        step->nullCheck = nullCheck;
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
ccnxTestrigScript_AddSendStep(CCNxTestrigScript *script, CCNxTestrigLinkID linkId, CCNxTlvDictionary *messageDictionary)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *step = _ccnxTestrigScriptStep_CreateSendStep(index, linkId, messageDictionary);
    parcLinkedList_Append(script->steps, step);
    return step;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddReceiveStep(CCNxTestrigScript *script, CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *step, char *failureMessage)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *newStep = _ccnxTestrigScriptStep_CreateReceiveStep(index, linkId, step, false, failureMessage);
    parcLinkedList_Append(script->steps, newStep);
    return newStep;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddNullReceiveStep(CCNxTestrigScript *script, CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *step, char *failureMessage)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *newStep = _ccnxTestrigScriptStep_CreateReceiveStep(index, linkId, step, true, failureMessage);
    parcLinkedList_Append(script->steps, newStep);
    return newStep;
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteSendStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    PARCBuffer *packetBuffer = ccnxTestrigPacketUtility_EncodePacket(step->packet);
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkByID(rig, step->linkId), packetBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(result, packetBuffer);
    return result;
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteReceiveStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    PARCBuffer *receiveBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkByID(rig, step->linkId), 1000);
    if (receiveBuffer == NULL && step->nullCheck) {
        ccnxTestrigSuiteTestResult_SetFail(result, step->failureMessage);
        return result;
    }

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

    return ccnxTestrigPacketUtility_IsValidPacketPair(referencedMessage, reconstructedMessage, result);
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    if (step->send) {
        return _ccnxTestrigScript_ExecuteSendStep(script, step, result, rig);
    } else {
        return _ccnxTestrigScript_ExecuteReceiveStep(script, step, result, rig);
    }
}

CCNxTestrigSuiteTestResult *
ccnxTestrigScript_Execute(CCNxTestrigScript *script, CCNxTestrig *rig)
{
    int numSteps = parcLinkedList_Size(script->steps);
    CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create(script->testCase);

    for (int i = 0; i < numSteps; i++) {
        printf(">> Executing step %d\n", i);
        CCNxTestrigScriptStep *step = parcLinkedList_GetAtIndex(script->steps, i);
        result = _ccnxTestrigScript_ExecuteStep(script, step, result, rig);

        // If the last step failed, stop the test and return the failure.
        if (ccnxTestrigSuiteTestResult_IsFailure(result)) {
            printf(">> **** Failed at step %d\n", i);
            return result;
        }
    }

    return ccnxTestrigSuiteTestResult_SetPass(result);
}
