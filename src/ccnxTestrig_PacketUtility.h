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

CCNxTestrigSuiteTestResult *ccnxTestrigPacketUtility_IsValidPacketPair(CCNxTlvDictionary *sent, CCNxMetaMessage *received, CCNxTestrigSuiteTestResult *result);

#endif // ccnxTestrig_PacketUtility_h
