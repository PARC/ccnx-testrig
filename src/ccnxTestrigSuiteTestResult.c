#include "ccnxTestrigSuiteTestResult.h"

struct ccnx_testrig_testresult {
    char *testCase;
    char *reason;
    bool passed;
};

static bool
_ccnxTestrigSuiteTestResult_Destructor(CCNxTestrigSuiteTestResult **resultPtr)
{
    CCNxTestrigSuiteTestResult *result = *resultPtr;

    // TODO

    return true;
}

parcObject_ImplementAcquire(ccnxTestrigSuiteTestResult, CCNxTestrigSuiteTestResult);
parcObject_ImplementRelease(ccnxTestrigSuiteTestResult, CCNxTestrigSuiteTestResult);

parcObject_Override(
	CCNxTestrigSuiteTestResult, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigSuiteTestResult_Destructor);

CCNxTestrigSuiteTestResult *
ccnxTestrigSuiteTestResult_CreatePass(char *testCase)
{
    CCNxTestrigSuiteTestResult *result = parcObject_CreateInstance(CCNxTestrigSuiteTestResult);
    if (result != NULL) {
        result->testCase = malloc(strlen(testCase));
        strcpy(result->testCase, testCase);
        result->passed = true;
    }
    return result;
}

CCNxTestrigSuiteTestResult *
ccnxTestrigSuiteTestResult_CreateFail(char *testCase, char *reason)
{
    CCNxTestrigSuiteTestResult *result = parcObject_CreateInstance(CCNxTestrigSuiteTestResult);
    if (result != NULL) {
        result->testCase = malloc(strlen(testCase));
        strcpy(result->testCase, testCase);
        result->reason = malloc(strlen(reason));
        strcpy(result->reason, reason);
        result->passed = false;
    }
    return result;
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
