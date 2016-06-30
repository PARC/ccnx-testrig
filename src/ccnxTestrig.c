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

static CCNxTestrigLink *
_ccnxTestrig_GetLinkA(CCNxTestrig *rig)
{
    return rig->linkA;
}

static CCNxTestrigLink *
_ccnxTestrig_GetLinkB(CCNxTestrig *rig)
{
    return rig->linkB;
}

static CCNxTestrigLink *
_ccnxTestrig_GetLinkC(CCNxTestrig *rig)
{
    return rig->linkC;
}

CCNxTestrigLink *
ccnxTestrig_GetLinkByID(CCNxTestrig *rig, CCNxTestrigLinkID linkID)
{
    switch (linkID) {
        case CCNxTestrigLinkID_LinkA:
            return _ccnxTestrig_GetLinkA(rig);
        case CCNxTestrigLinkID_LinkB:
            return _ccnxTestrig_GetLinkB(rig);
        case CCNxTestrigLinkID_LinkC:
            return _ccnxTestrig_GetLinkC(rig);
        default:
        break;
    }

    return NULL;
}

PARCBitVector *
ccnxTestrig_GetLinkVector(CCNxTestrig *rig, CCNxTestrigLinkID linkID, ...)
{
    PARCBitVector *vector = parcBitVector_Create();

    va_list linkList;
    va_start(linkList, linkID);
    parcBitVector_Set(vector, linkID);
    for (CCNxTestrigLinkID id = linkID; id != CCNxTestrigLinkID_NULL; id = va_arg(linkList, CCNxTestrigLinkID)) {
        parcBitVector_Set(vector, id);
    }

    return vector;
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
    int portNumber = options->port;
    char *address = options->address;
    CCNxTestrigLink *linkA = ccnxTestrigLink_Listen(options->linkType, address, portNumber++);
    printf("Link A created at %s:%04d\n", address, portNumber - 1);
    CCNxTestrigLink *linkB = ccnxTestrigLink_Listen(options->linkType, address, portNumber++);
    printf("Link B created at %s:%04d\n", address, portNumber - 1);
    CCNxTestrigLink *linkC = ccnxTestrigLink_Listen(options->linkType, address, portNumber++);
    printf("Link C created at %s:%04d\n", address, portNumber - 1);

    printf("Configure routes on the forwarder...\n");
    getc(stdin);

    // Create the test rig and save the links
    CCNxTestrig *testrig = ccnxTestrig_Create(options);
    testrig->linkA = linkA;
    testrig->linkB = linkB;
    testrig->linkC = linkC;

    // Run every test and disply the results
    ccnxTestrigSuite_RunAll(testrig);

    return 0;
}
