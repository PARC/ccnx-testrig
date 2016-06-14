#include "ccnxTestrig_PacketUtility.h"
#include "ccnxTestrig_SuiteTestResult.h"

#include <ccnx/common/codec/ccnxCodec_TlvPacket.h>

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

CCNxTestrigSuiteTestResult *
ccnxTestrigPacketUtility_IsValidPacketPair(CCNxTlvDictionary *sent, CCNxMetaMessage *received, CCNxTestrigSuiteTestResult *result)
{
    if (ccnxTlvDictionary_IsInterest(sent)) {
        CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(received);
        if (_validInterestPair(sent, receivedInterest) != CCNxInterestFieldError_None) {
            result = ccnxTestrigSuiteTestResult_SetFail(result, "An interest field was incorrect");
        }
    } else if (ccnxTlvDictionary_IsContentObject(sent)) {
        CCNxContentObject *receivedContent = ccnxMetaMessage_GetContentObject(received);
        if (_validContentPair(sent, received) != CCNxContentObjectFieldError_None) {
            result = ccnxTestrigSuiteTestResult_SetFail(result, "A content object field was incorrect");
        }
    } else if (ccnxTlvDictionary_IsManifest(sent)) {
        CCNxManifest *receivedManifest = ccnxMetaMessage_GetManifest(received);
        if (_validManifestPair(sent, receivedManifest) != CCNxManifestFieldError_None) {
            result = ccnxTestrigSuiteTestResult_SetFail(result, "A manifest field was incorrect.");
        }
    } else {
        ccnxTestrigSuiteTestResult_SetFail(result, "The sent and received packet pair did not have the same message type.");
    }

    return result;
}

PARCBuffer *
ccnxTestrigPacketUtility_EncodePacket(CCNxTlvDictionary *dict)
{
    CCNxCodecNetworkBufferIoVec *iovec = ccnxCodecTlvPacket_DictionaryEncode(dict, NULL);

    const struct iovec *array = ccnxCodecNetworkBufferIoVec_GetArray(iovec);
    size_t iovcnt = ccnxCodecNetworkBufferIoVec_GetCount(iovec);

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
