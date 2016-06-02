#include "ccnxTestrigSuiteTestResult.h"

#include <parc/algol/parc_LinkedList.h>

struct ccnx_testrig_testresult {
    char *testCase;
    char *reason;
    bool passed;
    PARCLinkedList *packetList;
};

static bool
_ccnxTestrigSuiteTestResult_Destructor(CCNxTestrigSuiteTestResult **resultPtr)
{
    CCNxTestrigSuiteTestResult *result = *resultPtr;

    if (result->packetList != NULL) {
        parcLinkedList_Release(&(result->packetList));
    }

    return true;
}

parcObject_ImplementAcquire(ccnxTestrigSuiteTestResult, CCNxTestrigSuiteTestResult);
parcObject_ImplementRelease(ccnxTestrigSuiteTestResult, CCNxTestrigSuiteTestResult);

parcObject_Override(
	CCNxTestrigSuiteTestResult, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigSuiteTestResult_Destructor);

CCNxTestrigSuiteTestResult *
ccnxTestrigSuiteTestResult_Create(char *testCase)
{
    CCNxTestrigSuiteTestResult *result = parcObject_CreateInstance(CCNxTestrigSuiteTestResult);

    if (result != NULL) {
        result->testCase = malloc(strlen(testCase));
        strcpy(result->testCase, testCase);
    }

    return result;
}

CCNxTestrigSuiteTestResult *
ccnxTestrigSuiteTestResult_SetPass(CCNxTestrigSuiteTestResult *testCase)
{
    testCase->passed = true;
    return testCase;
}

CCNxTestrigSuiteTestResult *
ccnxTestrigSuiteTestResult_SetFail(CCNxTestrigSuiteTestResult *testCase, char *reason)
{
    testCase->reason = malloc(strlen(reason));
    strcpy(testCase->reason, reason);
    testCase->passed = false;
    return testCase;
}

static void
_ccnxTestrigSuiteTestResult_Failed(CCNxTestrigSuiteTestResult *result, CCNxTestrigReporter *reporter)
{
    char *message = NULL;
    asprintf(&message, "Test %s FAIL: %s", result->testCase, result->reason);
    ccnxTestrigReporter_Report(reporter, message);
    free(message);
}

static void
_ccnxTestrigSuiteTestResult_Passed(CCNxTestrigSuiteTestResult *result, CCNxTestrigReporter *reporter)
{
    char *message = NULL;
    asprintf(&message, "Test %s PASS", result->testCase);
    ccnxTestrigReporter_Report(reporter, message);
    free(message);
}

void
ccnxTestrigSuiteTestResult_Report(CCNxTestrigSuiteTestResult *result, CCNxTestrigReporter *reporter)
{
    if (result->passed) {
        _ccnxTestrigSuiteTestResult_Passed(result, reporter);
    } else {
        _ccnxTestrigSuiteTestResult_Failed(result, reporter);
    }
}

void
ccnxTestrigSuiteTestResult_LogPacket(CCNxTestrigSuiteTestResult *testCase, PARCBuffer *packet)
{
    parcLinkedList_Append(testCase->packetList, packet);
}
