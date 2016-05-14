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

#include "ccnxTestrig.h"
#include "ccnxTestrig_Suite.h"

static PARCBuffer *
_encodeDictionary(const CCNxTlvDictionary *dict)
{
    // PARCSigner *signer = ccnxValidationCRC32C_CreateSigner();
    CCNxCodecNetworkBufferIoVec *iovec = ccnxCodecTlvPacket_DictionaryEncode((CCNxTlvDictionary *) dict, NULL);
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

void
ccnxTestrigSuite_TestBasicExchange(CCNxTestrig *rig)
{
    // Create the test packets
    CCNxName *testName = ccnxName_CreateFromCString("ccnx:/foo/bar");
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
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(ccnxTestrig_GetLinkC(rig), 1000);
    assertNotNull(receivedInterestBuffer, "Failed to receive a packet from the forwarder.");

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    assertTrue(ccnxMetaMessage_IsInterest(reconstructedInterest), "Expected the received message to be an interest.");

    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    // TODO: write comparisons for interest fields (hop count will be less!)
    // assertTrue(ccnxInterest_Equals(receivedInterest, interest), "Expected the received interest to match that which was sent.");

    ccnxTestrigLink_Send(ccnxTestrig_GetLinkC(rig), contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(ccnxTestrig_GetLinkC(rig));

    CCNxMetaMessage *reconstructedContent = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedContentBuffer);
    assertTrue(ccnxMetaMessage_IsContentObject(reconstructedContent), "Expected the received message to be a content object.");

    CCNxContentObject *receivedContent = ccnxMetaMessage_GetInterest(reconstructedContent);
    assertTrue(ccnxName_Equals(ccnxContentObject_GetName(receivedContent), ccnxContentObject_GetName(content)), "Expected the received content object names to match that which was sent.");
    // TODO: this failed the content object equals test

    parcBuffer_Display(contentBuffer, 0);
    parcBuffer_Display(receivedContentBuffer, 0);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    parcBuffer_Release(&interestBuffer);
    parcBuffer_Release(&contentBuffer);
    parcBuffer_Release(&receivedInterestBuffer);
    parcBuffer_Release(&receivedContentBuffer);

    ccnxInterest_Release(&receivedInterest);
    ccnxContentObject_Release(&receivedContent);

    parcBuffer_Release(&testPayload);
    ccnxName_Release(&testName);
}
