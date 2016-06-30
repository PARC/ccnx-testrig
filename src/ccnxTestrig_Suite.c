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
#include "ccnxTestrig_SuiteTestResult.h"
#include "ccnxTestrig_Script.h"

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
    parcSigner_Release(&signer);

    return buffer;
}

static PARCBuffer *
_computeMessageHash(CCNxTlvDictionary *dictionary)
{
    CCNxWireFormatMessageInterface *wireFacade = ccnxWireFormatMessageInterface_GetInterface(dictionary);
    PARCCryptoHash *hash = wireFacade->computeContentObjectHash(dictionary);

    PARCBuffer *digest = parcBuffer_Acquire(parcCryptoHash_GetDigest(hash));
    parcCryptoHash_Release(&hash);

    return digest;
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
ccnxTestrigSuite_FIBTest_BasicInterest_1a(CCNxTestrig *rig, char *testCaseName)
{
    // Create the protocol messages
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);
    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_FIBTest_BasicInterest_1b(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/c");
    assertNotNull(testName, "The name must not be NULL");

    // Create the protocol messages
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkC));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkC);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_1(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, manifest
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_2(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxManifest *manifest = ccnxManifest_Create(testName);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, manifest, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxManifest_Release(&manifest);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, dual path
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_3(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/bc");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB, CCNxTestrigLinkID_LinkC));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddRespondStep(script, step2, content);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, dual path with overlap
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_4(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/ab");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2a = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step2b = ccnxTestrigScript_AddReceiveNoneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddRespondStep(script, step2a, content);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, aggregation
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_5(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/c");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1a = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step1b = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step2a = ccnxTestrigScript_AddReceiveOneStep(script, step1a, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkC));
    CCNxTestrigScriptStep *step2b = ccnxTestrigScript_AddReceiveNoneStep(script, step1a, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkC));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddRespondStep(script, step2a, content);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveAllStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA, CCNxTestrigLinkID_LinkB));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, aggregation with passthrough
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTest_6(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/c");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1a = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2a = ccnxTestrigScript_AddReceiveOneStep(script, step1a, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkC));
    CCNxTestrigScriptStep *step1b = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2b = ccnxTestrigScript_AddReceiveOneStep(script, step1a, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkC));

    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddRespondStep(script, step2b, content);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveAllStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, wrong return, self-satisfied
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestErrors_1(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveNoneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, wrong return, invalid path
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestErrors_2(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkC);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveNoneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object, double answer
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestErrors_3(CCNxTestrig *rig, char *testCaseName)
{
    CCNxTestrigSuiteTestResult *testCase = ccnxTestrigSuiteTestResult_Create(testCaseName);

    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/c");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));
    CCNxTestrigScriptStep *step5 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step6 = ccnxTestrigScript_AddReceiveNoneStep(script, step5, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with hash restriction
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictions_1(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);
    PARCBuffer *hash = _computeMessageHash(content);

    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, hash);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with KeyId restriction
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictions_2(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    PARCBuffer *signatureBits = parcBuffer_Allocate(10); // arbitrary bufer size -- not important
    PARCSignature *signature = parcSignature_Create(PARCSigningAlgorithm_RSA, PARCCryptoHashType_SHA256, signatureBits);
    PARCBuffer *keyId = parcBuffer_Allocate(32);

    CCNxName *locatorName = ccnxName_CreateFromCString("ccnx:/key/locator");
    CCNxLink *keyURILink = ccnxLink_Create(locatorName, NULL, NULL);
    CCNxKeyLocator *keyLocator = ccnxKeyLocator_CreateFromKeyLink(keyURILink);

    ccnxContentObject_SetSignature(content, keyId, signature, keyLocator);

    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, keyId, NULL);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with Hash and KeyId restrictions
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictions_3(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    PARCBuffer *signatureBits = parcBuffer_Allocate(10); // arbitrary bufer size -- not important
    PARCSignature *signature = parcSignature_Create(PARCSigningAlgorithm_RSA, PARCCryptoHashType_SHA256, signatureBits);
    PARCBuffer *keyId = parcBuffer_Allocate(32);

    CCNxName *locatorName = ccnxName_CreateFromCString("ccnx:/key/locator");
    CCNxLink *keyURILink = ccnxLink_Create(locatorName, NULL, NULL);
    CCNxKeyLocator *keyLocator = ccnxKeyLocator_CreateFromKeyLink(keyURILink);

    ccnxContentObject_SetSignature(content, keyId, signature, keyLocator);
    PARCBuffer *hash = _computeMessageHash(content);

    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, keyId, hash);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request nameless object
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictions_4(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithPayload(testPayload);
    PARCBuffer *hash = _computeMessageHash(content);

    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, hash);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with hash restriction (invalid)
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictionErrors_1(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    PARCBuffer *hash = parcBuffer_Allocate(32); // this won't match the hash of the content object above
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, hash);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveNoneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with keyId restriction (invalid)
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictionErrors_2(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    PARCBuffer *signatureBits = parcBuffer_Allocate(10); // arbitrary bufer size -- not important
    PARCSignature *signature = parcSignature_Create(PARCSigningAlgorithm_RSA, PARCCryptoHashType_SHA256, signatureBits);
    PARCBuffer *keyId = parcBuffer_Allocate(32);

    CCNxName *locatorName = ccnxName_CreateFromCString("ccnx:/key/locator");
    CCNxLink *keyURILink = ccnxLink_Create(locatorName, NULL, NULL);
    CCNxKeyLocator *keyLocator = ccnxKeyLocator_CreateFromKeyLink(keyURILink);

    ccnxContentObject_SetSignature(content, keyId, signature, keyLocator);

    PARCBuffer *randomKeyId = parcBuffer_AllocateCString("this is a key id");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, randomKeyId, NULL);
    parcBuffer_Release(&randomKeyId);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with keyId restriction (invalid, missing)
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictionErrors_3(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    PARCBuffer *randomKeyId = parcBuffer_AllocateCString("this is a key id");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, randomKeyId, NULL);
    parcBuffer_Release(&randomKeyId);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with keyId restriction (invalid, no name)
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictionErrors_4(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithPayload(testPayload);

    PARCBuffer *signatureBits = parcBuffer_Allocate(10); // arbitrary bufer size -- not important
    PARCSignature *signature = parcSignature_Create(PARCSigningAlgorithm_RSA, PARCCryptoHashType_SHA256, signatureBits);
    PARCBuffer *keyId = parcBuffer_Allocate(32);

    CCNxName *locatorName = ccnxName_CreateFromCString("ccnx:/key/locator");
    CCNxLink *keyURILink = ccnxLink_Create(locatorName, NULL, NULL);
    CCNxKeyLocator *keyLocator = ccnxKeyLocator_CreateFromKeyLink(keyURILink);

    ccnxContentObject_SetSignature(content, keyId, signature, keyLocator);

    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, keyId, NULL);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with hash and keyId restriction (invalid hash)
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictionErrors_5(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithPayload(testPayload);

    PARCBuffer *signatureBits = parcBuffer_Allocate(10); // arbitrary bufer size -- not important
    PARCSignature *signature = parcSignature_Create(PARCSigningAlgorithm_RSA, PARCCryptoHashType_SHA256, signatureBits);
    PARCBuffer *keyId = parcBuffer_Allocate(32);

    CCNxName *locatorName = ccnxName_CreateFromCString("ccnx:/key/locator");
    CCNxLink *keyURILink = ccnxLink_Create(locatorName, NULL, NULL);
    CCNxKeyLocator *keyLocator = ccnxKeyLocator_CreateFromKeyLink(keyURILink);

    ccnxContentObject_SetSignature(content, keyId, signature, keyLocator);

    PARCBuffer *randomHash = parcBuffer_Allocate(32);
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, keyId, randomHash);
    parcBuffer_Release(&randomHash);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

// Request object with hash and keyId restriction (invalid keyID)
static CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_ContentObjectTestRestrictionErrors_6(CCNxTestrig *rig, char *testCaseName)
{
    // Create the test packets
    CCNxName *testName = _createRandomName("ccnx:/test/b");
    PARCBuffer *testPayload = parcBuffer_Allocate(1024);
    CCNxContentObject *content = ccnxContentObject_CreateWithPayload(testPayload);

    PARCBuffer *signatureBits = parcBuffer_Allocate(10); // arbitrary bufer size -- not important
    PARCSignature *signature = parcSignature_Create(PARCSigningAlgorithm_RSA, PARCCryptoHashType_SHA256, signatureBits);
    PARCBuffer *keyId = parcBuffer_Allocate(32);

    CCNxName *locatorName = ccnxName_CreateFromCString("ccnx:/key/locator");
    CCNxLink *keyURILink = ccnxLink_Create(locatorName, NULL, NULL);
    CCNxKeyLocator *keyLocator = ccnxKeyLocator_CreateFromKeyLink(keyURILink);

    ccnxContentObject_SetSignature(content, keyId, signature, keyLocator);
    PARCBuffer *hash = _computeMessageHash(content);

    PARCBuffer *randomKeyId = parcBuffer_AllocateCString("this is a key id");
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, randomKeyId, hash);
    parcBuffer_Release(&randomKeyId);

    CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
    CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, interest, CCNxTestrigLinkID_LinkA);
    CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveOneStep(script, step1, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkB));
    CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, content, CCNxTestrigLinkID_LinkB);
    CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveOneStep(script, step3, ccnxTestrig_GetLinkVector(rig, CCNxTestrigLinkID_LinkA));

    CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);

    ccnxInterest_Release(&interest);
    ccnxContentObject_Release(&content);

    ccnxName_Release(&testName);
    parcBuffer_Release(&testPayload);

    return testCaseResult;
}

static char *_testCaseNames[CCNxTestrigSuiteTest_LastEntry] = {
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1a",
    "CCNxTestrigSuiteTest_FIBTest_BasicInterest_1b",
    "CCNxTestrigSuiteTest_ContentObjectTest_1",
    "CCNxTestrigSuiteTest_ContentObjectTest_2",
    "CCNxTestrigSuiteTest_ContentObjectTest_3",
    "CCNxTestrigSuiteTest_ContentObjectTest_4",
    "CCNxTestrigSuiteTest_ContentObjectTest_5",
    "CCNxTestrigSuiteTest_ContentObjectTest_6",
    "CCNxTestrigSuiteTest_ContentObjectErrors_1",
    "CCNxTestrigSuiteTest_ContentObjectErrors_2",
    "CCNxTestrigSuiteTest_ContentObjectErrors_3",
    "CCNxTestrigSuiteTest_ContentObjectRestrictions_1",
    "CCNxTestrigSuiteTest_ContentObjectRestrictions_2",
    "CCNxTestrigSuiteTest_ContentObjectRestrictions_3",
    "CCNxTestrigSuiteTest_ContentObjectRestrictions_4",
    "CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_1",
    "CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_2",
    "CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_3",
    "CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_4",
    "CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_5",
    "CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_6",
};

CCNxTestrigSuiteTestResult *
ccnxTestrigSuite_RunTest(CCNxTestrig *rig, CCNxTestrigSuiteTest test)
{
    CCNxTestrigSuiteTestResult *result = NULL;

    switch (test) {
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1a:
            result = ccnxTestrigSuite_FIBTest_BasicInterest_1a(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_FIBTest_BasicInterest_1b:
            result = ccnxTestrigSuite_FIBTest_BasicInterest_1b(rig, _testCaseNames[test]);
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
        case CCNxTestrigSuiteTest_ContentObjectTest_5:
            result = ccnxTestrigSuite_ContentObjectTest_5(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectTest_6:
            result = ccnxTestrigSuite_ContentObjectTest_6(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectErrors_1:
            result = ccnxTestrigSuite_ContentObjectTestErrors_1(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectErrors_2:
            result = ccnxTestrigSuite_ContentObjectTestErrors_2(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectErrors_3:
            result = ccnxTestrigSuite_ContentObjectTestErrors_3(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictions_1:
            result = ccnxTestrigSuite_ContentObjectTestRestrictions_1(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictions_2:
            result = ccnxTestrigSuite_ContentObjectTestRestrictions_2(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictions_3:
            result = ccnxTestrigSuite_ContentObjectTestRestrictions_3(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictions_4:
            result = ccnxTestrigSuite_ContentObjectTestRestrictions_3(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_1:
            result = ccnxTestrigSuite_ContentObjectTestRestrictionErrors_1(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_2:
            result = ccnxTestrigSuite_ContentObjectTestRestrictionErrors_2(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_3:
            result = ccnxTestrigSuite_ContentObjectTestRestrictionErrors_3(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_4:
            result = ccnxTestrigSuite_ContentObjectTestRestrictionErrors_4(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_5:
            result = ccnxTestrigSuite_ContentObjectTestRestrictionErrors_5(rig, _testCaseNames[test]);
            break;
        case CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_6:
            result = ccnxTestrigSuite_ContentObjectTestRestrictionErrors_6(rig, _testCaseNames[test]);
            break;
        default:
            fprintf(stderr, "Error: unsupported test case: %d", test);
            break;
    }

    return result;
}

PARCLinkedList *
ccnxTestrigSuite_RunAll(CCNxTestrig *rig)
{
    PARCLinkedList *resultList = parcLinkedList_Create();
    CCNxTestrigReporter *reporter = ccnxTestrig_GetReporter(rig);

    for (int i = 0; i < CCNxTestrigSuiteTest_LastEntry; i++) {
        printf("Running test %d\n", i);
        CCNxTestrigSuiteTestResult *result = ccnxTestrigSuite_RunTest(rig, i);
        if (result != NULL) {
            ccnxTestrigSuiteTestResult_Report(result, reporter);
        }

        parcLinkedList_Append(resultList, result);
        ccnxTestrigSuiteTestResult_Release(&result);

        break;
    }

    return resultList;
}
