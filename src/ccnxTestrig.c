#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>

#include <stdbool.h>

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

#include "ccnxTestrig_Link.h"

typedef struct {
    CCNxTestrigLinkType linkType;
    char *fwdIPAddress;
    int fwdPort;
} CCNxTestrigOptions;

static bool
_ccnxTestrigOptions_Destructor(CCNxTestrigOptions **optionsPtr)
{
    CCNxTestrigOptions *options = *optionsPtr;
    return true;
}

parcObject_ImplementAcquire(ccnxTestrigOptions, CCNxTestrigOptions);
parcObject_ImplementRelease(ccnxTestrigOptions, CCNxTestrigOptions);

parcObject_Override(
	CCNxTestrigOptions, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigOptions_Destructor);

typedef struct {
    CCNxTestrigLink *link1;
    CCNxTestrigLink *link2;
    CCNxTestrigLink *link3;

    CCNxTestrigOptions *options;
} CCNxTestrig;

static bool
_ccnxTestrig_Destructor(CCNxTestrig **testrigPtr)
{
    CCNxTestrig *testrig = *testrigPtr;

    ccnxTestrigLink_Release(&testrig->link1);
    ccnxTestrigLink_Release(&testrig->link2);
    ccnxTestrigLink_Release(&testrig->link3);

    return true;
}

parcObject_ImplementAcquire(ccnxTestrig, CCNxTestrig);
parcObject_ImplementRelease(ccnxTestrig, CCNxTestrig);

parcObject_Override(
	CCNxTestrig, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrig_Destructor);

CCNxTestrig *
ccnxTestrig_Create(CCNxTestrigOptions *options)
{
    CCNxTestrig *testrig = parcObject_CreateInstance(CCNxTestrig);

    if (testrig != NULL) {
        testrig->options = ccnxTestrigOptions_Acquire(options);
        testrig->link1 = NULL;
        testrig->link2 = NULL;
        testrig->link3 = NULL;
    }

    return testrig;
}

void
showUsage()
{
    printf("Usage: ccnxTestrig <options> <FWD IP address> <port>\n");
    printf(" -t       --transport         Transport mechanism (0 = UDP, 1 = TCP, 2 = ETH)\n");
    printf(" -h       --help              Display the help message\n");
}

static CCNxTestrigOptions *
_ccnxTestrig_ParseCommandLineOptions(int argc, char **argv)
{
    static struct option longopts[] = {
            { "transport",  required_argument,  NULL,'t' },
            // { "window",     required_argument,  NULL,'w'},
            { "help",       no_argument,        NULL,'h'},
            { NULL,0,NULL,0}
    };

    if (argc < 2) {
        showUsage();
        exit(EXIT_FAILURE);
    }

    CCNxTestrigOptions *options = parcObject_CreateInstance(CCNxTestrigOptions);
    options->fwdPort = 0;
    options->fwdIPAddress = NULL;

    int c;
    while (optind < argc) {
        if ((c = getopt_long(argc, argv, "ht:", longopts, NULL)) != -1) {
            switch(c) {
                case 't':
                    sscanf(optarg, "%zu", (size_t *) &(options->linkType));
                    break;
                case 'h':
                    showUsage();
                    exit(EXIT_SUCCESS);
                default:
                    break;
            }
        } else { // handle the rest of the mandatory options
            asprintf(&(options->fwdIPAddress), "%s", argv[optind++]);
            options->fwdPort = atoi(argv[optind++]);
        }
    }

    if (options->fwdPort == 0 || options->fwdIPAddress == NULL) {
        showUsage();
        exit(EXIT_FAILURE);
    }

    return options;
};

// static CCNxContentObject *
// _createReceivedContent(CCNxContentObject *preSendContent)
// {
//     PARCSigner *signer = ccnxValidationCRC32C_CreateSigner();
//     CCNxCodecNetworkBufferIoVec *iovec = ccnxCodecTlvPacket_DictionaryEncode(preSendContent, signer);
//     assertTrue(ccnxWireFormatMessage_PutIoVec(preSendContent, iovec), "ccnxWireFormatMessage_PutIoVec failed");;
//     parcSigner_Release(&signer);
//
//     PARCBuffer *encodedBuffer = _iovecToBuffer(iovec);
//
//     CCNxContentObject *postSendContent = ccnxMetaMessage_CreateFromWireFormatBuffer(encodedBuffer);
//
//     parcBuffer_Release(&encodedBuffer);
//
//     return postSendContent;
// }
//
// static PARCBuffer *
// _createMessageHash(const CCNxMetaMessage *metaMessage)
// {
//     CCNxWireFormatMessage *wireFormatMessage = (CCNxWireFormatMessage *) metaMessage;
//
//     PARCCryptoHash *hash = ccnxWireFormatMessage_CreateContentObjectHash(wireFormatMessage);
//     PARCBuffer *buffer = parcBuffer_Acquire(parcCryptoHash_GetDigest(hash));
//     parcCryptoHash_Release(&hash);
//
//     return buffer;
// }

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

static void
_ccnxTestrig_TestBasicExchange(CCNxTestrig *rig)
{
    // Create the test packets
    CCNxName *testName = ccnxName_CreateFromCString("ccnx:/test/_ccnxTestrig_TestBasicExchange");
    assertNotNull(testName, "The name must not be NULL");
    PARCBuffer *testPayload = parcBuffer_WrapCString("_ccnxTestrig_TestBasicExchange");

    // Create the protocol messages
    CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
    CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

    // Stamp wire format representations
    PARCBuffer *interestBuffer = _encodeDictionary(interest);
    PARCBuffer *contentBuffer = _encodeDictionary(content);

    // Send the interest to the forwarder.
    printf("Sending the first interest\n");
    ccnxTestrigLink_Send(rig->link1, interestBuffer);
    printf("Waiting for the response\n");
    PARCBuffer *receivedInterestBuffer = ccnxTestrigLink_ReceiveWithTimeout(rig->link3, 1000);

    // Verify that the interest is correct
    CCNxMetaMessage *reconstructedInterest = ccnxMetaMessage_CreateFromWireFormatBuffer(receivedInterestBuffer);
    assertTrue(ccnxMetaMessage_IsInterest(reconstructedInterest), "Expected the received message to be an interest.");

    CCNxInterest *receivedInterest = ccnxMetaMessage_GetInterest(reconstructedInterest);
    // TODO: write comparisons for interest fields (hop count will be less!)
    // assertTrue(ccnxInterest_Equals(receivedInterest, interest), "Expected the received interest to match that which was sent.");

    ccnxTestrigLink_Send(rig->link3, contentBuffer);
    PARCBuffer *receivedContentBuffer = ccnxTestrigLink_Receive(rig->link1);

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

int
main(int argc, char** argv)
{
    // Parse options and create the test rig
    CCNxTestrigOptions *options = _ccnxTestrig_ParseCommandLineOptions(argc, argv);
    CCNxTestrig *testrig = ccnxTestrig_Create(options);

    // Open connections to forwarder
    testrig->link1 = ccnxTestrigLink_Connect(options->linkType, options->fwdIPAddress, options->fwdPort);
    printf("Link 1 created\n");
    testrig->link2 = ccnxTestrigLink_Connect(options->linkType, options->fwdIPAddress, options->fwdPort + 1);
    printf("Link 2 created\n");
    testrig->link3 = ccnxTestrigLink_Listen(options->linkType, "localhost", options->fwdPort + 2);
    printf("Link 3 created\n");

    // Run each test and disply the results
    _ccnxTestrig_TestBasicExchange(testrig);
    // ...

    return 0;
}
