#include "ccnxTestrig_SuiteTestResult.h"
#include "ccnxTestrig_Script.h"

#include <parc/algol/parc_LinkedList.h>

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
_ccnxTestrigScriptStep_CreateReceiveStep(int index, CCNxTestrigLinkID linkId, CCNxTlvDictionary *messageDictionary,
    CCNxTestrigScriptStep *reference, bool nullCheck, char *failureMessage)
{
    CCNxTestrigScriptStep *step = parcObject_CreateInstance(CCNxTestrigScriptStep);
    if (step != NULL) {
        step->stepIndex = index;
        step->linkId = linkId;
        step->packet = ccnxTlvDictionary_Acquire(messageDictionary);
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
    CCNxTestrigScriptStep *step = _ccnxTestrigScriptStep_CreateReceiveStep(index, linkId, messageDictionary, false, failureMessage);
    parcLinkedList_Append(script->steps, step);
    return step;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddNullReceiveStep(CCNxTestrigScript *script, CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *step, char *failureMessage)
{
    size_t index = parcLinkedList_Size(script->steps);
    CCNxTestrigScriptStep *step = _ccnxTestrigScriptStep_CreateReceiveStep(index, linkId, messageDictionary, true, failureMessage);
    parcLinkedList_Append(script->steps, step);
    return step;
}

static PARCBuffer *
_encodeDictionary(const CCNxTlvDictionary *dict)
{
    PARCSigner *signer = ccnxValidationCRC32C_CreateSigner();
    CCNxCodecNetworkBufferIoVec *iovec = ccnxCodecTlvPacket_DictionaryEncode((CCNxTlvDictionary *) dict, signer);
    const struct iovec *array = ccnxCodecNetworkBufferIoVec_GetArray(iovec);
    size_t iovcnt = ccnxCodecNetworkBufferIoVec_GetCount((CCNxCodecNetworkBufferIoVec *) iovec);

    size_t totalbytes = 0;
    for (int i = 0; i < iovcnt; i++) {
        totalbytes += array[i].iov_len;
    }
    PARCBuffer *buffer = parcBuffer_Allocate(totalbytes);
    for (int i = 0; i < iovcnt; i++) {
        parcBuffer_PutArray(buffer, array[i].iov_len, array[i].iov_base);
    }
    parcBuffer_Flip(buffer);

    ccnxCodecNetworkBufferIoVec_Release(&iovec);
    parcSigner_Release(&signer);

    return buffer;
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteSendStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, CCNxTestrigSuiteTestResult *result, CCNxTestrig *rig)
{
    PARCBuffer *packetBuffer = _encodeDictionary(step->packet);
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

    return _ccnxTestrigPacketUtility_IsValidPacketPair(referencedMessage, reconstructedMessage);
}

static CCNxTestrigSuiteTestResult *
_ccnxTestrigScript_ExecuteStep(CCNxTestrigScript *script, CCNxTestrigScriptStep *step, CCNxTestrig *rig)
{
    if (step->send) {
        return _ccnxTestrigScript_ExecuteSendStep(script, step, rig);
    } else {
        return _ccnxTestrigScript_ExecuteReceiveStep(script, step, rig);
    }
}

CCNxTestrigSuiteTestResult *
ccnxTestrigScript_Execute(CCNxTestrigScript *script, CCNxTestrig *rig)
{
    size_t numSteps = parcLinkedList_Size(script->steps);
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    for (size_t i = 0; i < numSteps; i++) {
        CCNxTestrigScriptStep *step = parcLinkedList_Get(script->steps, i);
        testCaseResult = _ccnxTestrigScript_ExecuteStep(script, step, testCaseResult, rig);

        // If the last step failed, stop the test and return the failure.
        if (ccnxTestrigSuiteTestResult_IsFailure(testCaseResult)) {
            return testCaseResult;
        }
    }

    return ccnxTestrigSuiteTestResult_SetPass(testCaseResult);
}
