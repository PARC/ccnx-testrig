#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
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

#include "ccnxTestrig_Suite.h"
#include "ccnxTestrig_Reporter.h"

#define DEFAULT_PORT 9596
#define DEFAULT_ADDRESS "localhost"

typedef struct {
    CCNxTestrigLinkType linkType;

    char *address;
    int port;
} _CCNxTestrigOptions;

static bool
_CCNxTestrigOptions_Destructor(_CCNxTestrigOptions **optionsPtr)
{
    _CCNxTestrigOptions *options = *optionsPtr;

    free(options->address);

    return true;
}

parcObject_ImplementAcquire(_ccnxTestrigOptions, _CCNxTestrigOptions);
parcObject_ImplementRelease(_ccnxTestrigOptions, _CCNxTestrigOptions);

parcObject_Override(
	_CCNxTestrigOptions, PARCObject,
	.destructor = (PARCObjectDestructor *) _CCNxTestrigOptions_Destructor);

struct ccnx_testrig {
    CCNxTestrigLink *linkA;
    CCNxTestrigLink *linkB;
    CCNxTestrigLink *linkC;

    _CCNxTestrigOptions *options;
    CCNxTestrigReporter *reporter;
};

static bool
_ccnxTestrig_Destructor(CCNxTestrig **testrigPtr)
{
    CCNxTestrig *testrig = *testrigPtr;

    ccnxTestrigLink_Release(&testrig->linkA);
    ccnxTestrigLink_Release(&testrig->linkB);
    ccnxTestrigLink_Release(&testrig->linkC);

    _ccnxTestrigOptions_Release(&testrig->options);

    return true;
}

parcObject_ImplementAcquire(ccnxTestrig, CCNxTestrig);
parcObject_ImplementRelease(ccnxTestrig, CCNxTestrig);

parcObject_Override(
	CCNxTestrig, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrig_Destructor);

CCNxTestrig *
ccnxTestrig_Create(_CCNxTestrigOptions *options)
{
    CCNxTestrig *testrig = parcObject_CreateInstance(CCNxTestrig);

    if (testrig != NULL) {
        testrig->options = _ccnxTestrigOptions_Acquire(options);
        testrig->reporter = ccnxTestrigReporter_Create(stdout);
    }

    return testrig;
}

CCNxTestrigReporter *
ccnxTestrig_GetReporter(CCNxTestrig *rig)
{
    return rig->reporter;
}

CCNxTestrigLink *
ccnxTestrig_GetLinkByID(CCNxTestrig *rig, CCNxTestrigLinkID linkID)
{
    switch (linkID) {
        case CCNxTestrigLinkID_LinkA:
            return ccnxTestrig_GetLinkA(rig);
        case CCNxTestrigLinkID_LinkB:
            return ccnxTestrig_GetLinkB(rig);
        case CCNxTestrigLinkID_LinkC:
            return ccnxTestrig_GetLinkC(rig);
    }

    return NULL;
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

void
showUsage()
{
    printf("Usage: ccnxTestrig [-h] [-t (UDP | TCP)] [-a <local address>] [-p <local port>] \n");
    printf(" -a       --address           Local IP address (localhost by default)\n");
    printf(" -p       --port              Local IP port (9596 by defualt)\n");
    printf(" -t       --transport         Transport mechanism (0 = UDP, 1 = TCP)\n");
    printf(" -h       --help              Display the help message\n");
}

static _CCNxTestrigOptions *
_ccnxTestrig_ParseCommandLineOptions(int argc, char **argv)
{
    static struct option longopts[] = {
            { "address",    required_argument,  NULL, 'a'},
            { "port",       required_argument,  NULL, 'p'},
            { "transport",  required_argument,  NULL, 't' },
            { "help",       no_argument,        NULL, 'h'},
            { NULL,         0,                  NULL, 0}
    };

    if (argc < 2) {
        showUsage();
        exit(EXIT_FAILURE);
    }

    _CCNxTestrigOptions *options = parcObject_CreateInstance(_CCNxTestrigOptions);
    options->port = 0;
    options->address = NULL;

    int c;
    while (optind < argc) {
        if ((c = getopt_long(argc, argv, "ht:a:p:", longopts, NULL)) != -1) {
            switch(c) {
                case 't':
                    sscanf(optarg, "%zu", (size_t *) &(options->linkType));
                    break;
                case 'a':
                    options->address = malloc(strlen(optarg));
                    strcpy(options->address, optarg);
                    break;
                case 'p':
                    sscanf(optarg, "%zu", (size_t *) &(options->port));
                    break;
                case 'h':
                    showUsage();
                    exit(EXIT_SUCCESS);
                default:
                    break;
            }
        }
    }

    // Configure the defaults.
    if (options->port == 0) {
        options->port = DEFAULT_PORT;
    }
    if (options->address == NULL) {
        options->address = malloc(strlen(DEFAULT_ADDRESS));
        strcpy(options->address, DEFAULT_ADDRESS);
    }

    return options;
};

int
main(int argc, char** argv)
{
    // Parse options and create the test rig
    _CCNxTestrigOptions *options = _ccnxTestrig_ParseCommandLineOptions(argc, argv);

    // Open connections to the forwarder
    size_t portNumber = options->port;
    char *address = options->address;
    CCNxTestrigLink *linkA = ccnxTestrigLink_Listen(options->linkType, address, portNumber++);
    printf("Link A created\n");
    CCNxTestrigLink *linkB = ccnxTestrigLink_Listen(options->linkType, address, portNumber++);
    printf("Link B created\n");
    CCNxTestrigLink *linkC = ccnxTestrigLink_Listen(options->linkType, address, portNumber++);
    printf("Link C created\n");

    // Create the test rig and save the links
    CCNxTestrig *testrig = ccnxTestrig_Create(options);
    testrig->linkA = linkA;
    testrig->linkB = linkB;
    testrig->linkC = linkC;

    // Run every test and disply the results
    ccnxTestrigSuite_RunAll(testrig);

    return 0;
}
