#include <LongBow/runtime.h>

#include <parc/algol/parc_Object.h>
#include <parc/security/parc_Signer.h>

#include <ccnx/common/ccnx_Name.h>
#include <ccnx/common/ccnx_Interest.h>
#include <ccnx/common/ccnx_ContentObject.h>

#include <ccnx/common/codec/ccnxCodec_NetworkBuffer.h>
#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>

#include <ccnx/transport/common/transport_MetaMessage.h>
#include <ccnx/transport/common/transport_Message.h>

#include <ccnx/common/validation/ccnxValidation_CRC32C.h>

#include "ccnxTestrig.h"
#include "ccnxTestrig_Suite.h"

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

static CCNxInterestFieldError
_validInterestPair(CCNxInterest *egress, CCNxInterest *ingress)
{
    if (ccnxInterest_GetHopLimit(egress) != ccnxInterest_GetHopLimit(ingress) + 1) {
        return CCNxInterestFieldError_HopLimit;
    }

    if (ccnxInterest_GetLifetime(egress) != ccnxInterest_GetLifetime(ingress)) {
        return CCNxInterestFieldError_Lifetime;
    }

    if (!ccnxName_Equals(ccnxInterest_GetName(egress), ccnxInterest_GetName(ingress))) {
        return CCNxInterestFieldError_Name;
    }

    if (!parcBuffer_Equals(ccnxInterest_GetKeyIdRestriction(egress), ccnxInterest_GetKeyIdRestriction(ingress))) {
        return CCNxInterestFieldError_KeyIdRestriction;
    }

    if (!parcBuffer_Equals(ccnxInterest_GetContentObjectHashRestriction(egress), ccnxInterest_GetContentObjectHashRestriction(ingress))) {
        return CCNxInterestFieldError_ContentObjectHashRestriction;
    }

    return CCNxInterestFieldError_None;
}

static CCNxContentObjectFieldError
_validContentPair(CCNxContentObject *egress, CCNxContentObject *ingress)
{
    return CCNxContentObjectFieldError_None;
}

static CCNxManifestFieldError
_validManifestPair(CCNxManifest *egress, CCNxManifest *ingress)
{
    return CCNxManifestFieldError_None;
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

    return buffer;
}

static CCNxName *
_createRandomName(char *prefix)
{
    int nonce = rand();
    CCNxName *name = ccnxName_CreateFromCString(prefix);
    char *suffix = NULL;
    asprintf(&suffix, "%d", nonce);

    CCNxName *full = ccnxName_ComposeNAME(name, suffix);
    free(suffix);
    ccnxName_Release(&name);

    return full;
}

static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_FIBTest_SimpleExchange(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/foo/bar");
    assertNotNull(testName, "The name must not be NULL");
    PARCBuffer *testPayload = parcBuffer_WrapCString("_ccnxTestrig_TestBasicExchange");

    // Create the protocol messages
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder.
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
        return failure;
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }

    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    ccnxTestrigLink_Send(ccnxTestrig_GetLinkC(rig), contentBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkA(rig));

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBuffer);
    if (ccnxMetaMessage_IsContentObject(reconstructedContent)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(reconstructedContent);
    if (_validContentPair(content, receivedContent) != CCNxContentObjectFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBuffer);

    ccnxMetaMessage_Release(&reconstructedContent);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    parcBuffer_Release(&testPayload);
    ccnxName_Release(&testName);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_FIBTest_BasicInterest_1a(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    assertNotNull(testName, "The name must not be NULL");

    // Create the protocol messages
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder.
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkB(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
        return failure;
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }
    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    ccnxTestrigLink_Send(ccnxTestrig_GetLinkB(rig), contentBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkA(rig));

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBuffer);
    if (ccnxMetaMessage_IsContentObject(reconstructedContent)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(reconstructedContent);
    if (_validContentPair(content, receivedContent) != CCNxContentObjectFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBuffer);

    ccnxMetaMessage_Release(&reconstructedContent);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_FIBTest_BasicInterest_1b(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/c");
    assertNotNull(testName, "The name must not be NULL");

    // Create the protocol messages
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder.
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
        return failure;
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }
    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    ccnxTestrigLink_Send(ccnxTestrig_GetLinkC(rig), contentBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkA(rig));

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBuffer);
    if (ccnxMetaMessage_IsContentObject(reconstructedContent)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(reconstructedContent);
    if (_validContentPair(content, receivedContent) != CCNxContentObjectFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBuffer);

    ccnxMetaMessage_Release(&reconstructedContent);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

// Request object
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_1(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder.
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
        return failure;
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }
    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    ccnxTestrigLink_Send(ccnxTestrig_GetLinkC(rig), contentBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkA(rig));

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBuffer);
    if (ccnxMetaMessage_IsContentObject(reconstructedContent)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(reconstructedContent);
    if (_validContentPair(content, receivedContent) != CCNxContentObjectFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBuffer);

    ccnxMetaMessage_Release(&reconstructedContent);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

// Request object, manifest
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_2(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxManifest *manifest = ccnxManifest_Create(testName);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *manifestBuffer = _encodeDictionary(manifest);

    // Send the interest to the forwarder.
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
        return failure;
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }
    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    ccnxTestrigLink_Send(ccnxTestrig_GetLinkC(rig), manifestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, manifestBuffer);
    PARCBuffer *receivedManifestBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkA(rig));

    CCNxMetaMessage *reconstructedManifest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedManifestBuffer);
    if (ccnxMetaMessage_IsManifest(reconstructedManifest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a manifest.");
        return failure;
    }

    CCNxManifest *receivedManifest = ccnxMetaMessage_GetManifest(reconstructedManifest);
    if (_validManifestPair(manifest, receivedManifest) != CCNxManifestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a manifest.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxManifest_Release(&manifest);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&manifestBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedManifestBuffer);

    ccnxMetaMessage_Release(&reconstructedManifest);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedManifest);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

// Request object, dual path
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_3(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/bc");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder.
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);

    // Receive the interest on links B or C.
    bool linkC = true;
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        linkC = false;
        receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
        if (receivedInterestBuffer == NULL) {
            CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
            return failure;
        }
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }
    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    // Send the content back to the right downstream link
    if (linkC) {
        ccnxTestrigLink_Send(ccnxTestrig_GetLinkC(rig), contentBuffer);
    } else {
        ccnxTestrigLink_Send(ccnxTestrig_GetLinkB(rig), contentBuffer);
    }
    ccnxTestrigSuiteTestResult_LogPacket(testCase, contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkA(rig));

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBuffer);
    if (ccnxMetaMessage_IsContentObject(reconstructedContent)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(reconstructedContent);
    if (_validContentPair(content, receivedContent) != CCNxContentObjectFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBuffer);

    ccnxMetaMessage_Release(&reconstructedContent);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

// Request object, dual path with overlap
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_4(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/ab");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder.
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);

    // Receive the interest on links B -- not link A
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
        return failure;
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }
    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    // Send the content back to the right downstream link
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkB(rig), contentBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkA(rig));

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBuffer);
    if (ccnxMetaMessage_IsContentObject(reconstructedContent)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(reconstructedContent);
    if (_validContentPair(content, receivedContent) != CCNxContentObjectFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBuffer);

    ccnxMetaMessage_Release(&reconstructedContent);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

// Request object, aggregation
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_5(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/c");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder on link A
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkA(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);

    // Send the interest to the forwarder on link B, and cause aggregation
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkB(rig), interestBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, interestBuffer);

    // Receive the interest on C
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer == NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Failed to receive a packet from the forwarder.");
        return failure;
    }
    receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    if (receivedInterestBuffer != NULL) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Received two packets from the forwarder (interests were not aggregated correctly).");
        return failure;
    }

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    if (ccnxMetaMessage_IsInterest(reconstructedInterest)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be an interest.");
        return failure;
    }
    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    CCNxInterestFieldError interestError = _validInterestPair(interest, receivedInterest);
    if (interestError != CCNxInterestFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "An interest field was incorrect");
        return failure;
    }

    // Send the content back to the right downstream link
    ccnxTestrigLink_Send(ccnxTestrig_GetLinkC(rig), contentBuffer);
    ccnxTestrigSuiteTestResult_LogPacket(testCase, contentBuffer);

    // Receive the content on both links
    PARCBuffer *receivedContentBufferA = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkA(rig), 1000);
    PARCBuffer *receivedContentBufferB = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkB(rig), 1000);

    assertTrue(parcBuffer_Equals(receivedContentBufferA, receivedContentBufferA), "Expected the received content objects to be equal.");

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBufferA);
    if (ccnxMetaMessage_IsContentObject(reconstructedContent)) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(reconstructedContent);
    if (_validContentPair(content, receivedContent) != CCNxContentObjectFieldError_None) {
        CCNxTestrigSuiteTestResult *failure = ccnxTestrigSuiteTestResult_SetFail(testCase, "Expected the received message to be a content object.");
        return failure;
    }

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBufferB);
    parcBuffer_Release(&receivedContentBufferA);

    ccnxMetaMessage_Release(&reconstructedContent);
    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    CCNxTestrigSuiteTestResult *testResult = ccnxTestrigSuiteTestResult_SetPass(testCase);
    return testResult;
}

static char *_testCaseNames[CCNxTestrigSuiteTest_LastEntry] = {
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1a",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1b",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1c",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1d",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1e",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1f",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_2a",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_2b",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_2c",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_3a",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_3b",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_3c",
    "CCNxTestrigSuiteTest_PITTest_1",
    "CCNxTestrigSuiteTest_CSTest_1",
    "CCNxTestrigSuiteTest_RequestTest_Standard_1",
    "CCNxTestrigSuiteTest_RequestTest_Errors_1",
    "CCNxTestrigSuiteTest_RequestTest_Restrictions_1",
    "CCNxTestrigSuiteTest_ContentObjectTest_1",
    "CCNxTestrigSuiteTest_ContentObjectTest_2",
    "CCNxTestrigSuiteTest_ContentObjectTest_3",
    "CCNxTestrigSuiteTest_ContentObjectTest_4"
};

CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_RunTest(CCNxTestrig *rig, CCNxTestrigSuiteTest test)
{
    CCNxTestrigSuiteTestResult *result = NULL;

    switch (test) {
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1a:
            result = ccnxTestrigSuite_FIBTest_BasicInterest_1a(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectTest_1:
            result = ccnxTestrigSuite_ContentObjectTest_1(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectTest_2:
            result = ccnxTestrigSuite_ContentObjectTest_2(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectTest_3:
            result = ccnxTestrigSuite_ContentObjectTest_3(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectTest_4:
            result = ccnxTestrigSuite_ContentObjectTest_4(rig, _testCaseNames[test]);
            break;

        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1b:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1c:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1d:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1e:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1f:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_2a:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_2b:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_2c:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_3a:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_3b:
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_3c:
        case CCNxTestrigSuiteTest_PITTest_1:
        case CCNxTestrigSuiteTest_CSTest_1:
        case CCNxTestrigSuiteTest_RequestTest_Standard_1:
        case CCNxTestrigSuiteTest_RequestTest_Errors_1:
        case CCNxTestrigSuiteTest_RequestTest_Restrictions_1:
        default:
            break;
    }

    return result;
}

void
ccnxTestrigSuite_RunAll(CCNxTestrig *rig)
{
    CCNxTestrigReporter *reporter = ccnxTestrig_GetReporter(rig);

    for (int i = 0; i < CCNxTestrigSuiteTest_LastEntry; i++) {
        CCNxTestrigSuiteTestResult *result = ccnxTestrigSuite_RunTest(rig, i);
        if (result != NULL) {
            ccnxTestrigSuiteTestResult_Report(result, reporter);
        }
    }
}
