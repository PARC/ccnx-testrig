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

#include "link.h"

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

    int c;
    while (optind < argc) {
        if ((c = getopt_long(argc, argv, "ht:", longopts, NULL)) != -1) {
            switch(c) {
                case 't':
                    sscanf(optarg, "%zu", &(options->transportType));
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

    return options;
};

static void
_ccnxTestrig_TestBasicExchange(CCNxTestrig *rig)
{
    // TODO: implement me.
}

int
main(int argc, char** argv)
{
    // Parse options and create the test rig
    CCNxTestrigOptions *options = _ccnxTestrig_ParseCommandLineOptions(argc, argv);
    CCNxTestrig *testrig = ccnxTestrig_Create(CCNxTestrigOptions *options);

    // Open connections to forwarder
    testrig->link1 = ccnxTestrigLink_Connect(options->linkType, testrig->fwdIPAddress, testrig->fwdPort);
    testrig->link2 = ccnxTestrigLink_Connect(options->linkType, testrig->fwdIPAddress, testrig->fwdPort + 1);
    testrig->link3 = ccnxTestrigLink_Listen(options->linkType, "localhost", testrig->fwdPort + 2);

    // Wait for the trigger to run the tests
    // TODO: is there a better way to trigger this?
    getc();

    // Run each test and disply the results
    // TODO



    return 0;
}
