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

#include "ccnxTestrig.h"

typedef struct {
    CCNxTestrigLinkType linkType;
    char *fwdIPAddress;
    int fwdPort;
} _CCNxTestrigOptions;

static bool
__CCNxTestrigOptions_Destructor(_CCNxTestrigOptions **optionsPtr)
{
    _CCNxTestrigOptions *options = *optionsPtr;
    return true;
}

parcObject_ImplementAcquire(_CCNxTestrigOptions, _CCNxTestrigOptions);
parcObject_ImplementRelease(_CCNxTestrigOptions, _CCNxTestrigOptions);

parcObject_Override(
	_CCNxTestrigOptions, PARCObject,
	.destructor = (PARCObjectDestructor *) __CCNxTestrigOptions_Destructor);

struct ccnx_testrig {
    CCNxTestrigLink *linkA;
    CCNxTestrigLink *linkB;
    CCNxTestrigLink *linkC;

    _CCNxTestrigOptions *options;
};

static bool
_ccnxTestrig_Destructor(CCNxTestrig **testrigPtr)
{
    CCNxTestrig *testrig = *testrigPtr;

    ccnxTestrigLink_Release(&testrig->linkA);
    ccnxTestrigLink_Release(&testrig->linkB);
    ccnxTestrigLink_Release(&testrig->linkC);

    return true;
}

parcObject_ImplementAcquire(ccnxTestrig, CCNxTestrig);
parcObject_ImplementRelease(ccnxTestrig, CCNxTestrig);

parcObject_Override(
	CCNxTestrig, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrig_Destructor);

CCNxTestrig *
ccnxTestrig_Create(_CCNxTestrigOptions *options, CCNxTestrigLink *linkA, CCNxTestrigLink *linkB, CCNxTestrigLink *linkC)
{
    CCNxTestrig *testrig = parcObject_CreateInstance(CCNxTestrig);

    if (testrig != NULL) {
        testrig->options = _CCNxTestrigOptions_Acquire(options);
        testrig->linkA = ccnxTestrigLink_Acquire(linkA);
        testrig->linkB = ccnxTestrigLink_Acquire(linkB);
        testrig->linkC = ccnxTestrigLink_Acquire(linkC);
    }

    return testrig;
}

CCNxTestrigLink *
ccnxTestrig_GetLinkA(CCNxTestrig *rig)
{
    return rig->linkA;
}

CCNxTestrigLink *
ccnxTestrig_GetLinkB(CCNxTestrig *rig)
{
    return rig->linkB;
}

CCNxTestrigLink *
ccnxTestrig_GetLinkC(CCNxTestrig *rig)
{
    return rig->linkC;
}

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

void
showUsage()
{
    printf("Usage: ccnxTestrig <options> <FWD IP address> <port>\n");
    printf(" -t       --transport         Transport mechanism (0 = UDP, 1 = TCP, 2 = ETH)\n");
    printf(" -h       --help              Display the help message\n");
}

static _CCNxTestrigOptions *
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

    _CCNxTestrigOptions *options = parcObject_CreateInstance(_CCNxTestrigOptions);
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

int
main(int argc, char** argv)
{
    // Parse options and create the test rig
    _CCNxTestrigOptions *options = _ccnxTestrig_ParseCommandLineOptions(argc, argv);

    // Open connections to forwarder
    CCNxTestrigLink *linkA = ccnxTestrigLink_Connect(options->linkType, options->fwdIPAddress, options->fwdPort);
    printf("Link 1 created\n");
    CCNxTestrigLink *linkB = ccnxTestrigLink_Connect(options->linkType, options->fwdIPAddress, options->fwdPort + 1);
    printf("Link 2 created\n");
    CCNxTestrigLink *linkC = ccnxTestrigLink_Listen(options->linkType, "localhost", options->fwdPort + 2);
    printf("Link 3 created\n");

    // Create the test rig with these links
    CCNxTestrig *testrig = ccnxTestrig_Create(options, linkA, linkB, linkC);

    // Run each test and disply the results
    _ccnxTestrig_TestBasicExchange(testrig);
    // ... insert other tests here

    return 0;
}
