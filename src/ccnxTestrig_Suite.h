#ifndef ccnx_testrig_suite_h
#define ccnx_testrig_suite_h

#include "ccnxTestrig.h"
#include "ccnxTestrigSuiteTestResult.h"

typedef enum {
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1a = 0x00,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1b,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1c,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1d,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1e,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1f,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_2a,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_2b,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_2c,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_3a,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_3b,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_3c,

    CCNxTestrigSuiteTest_PITTest_1,

    CCNxTestrigSuiteTest_CSTest_1,

    CCNxTestrigSuiteTest_RequestTest_Standard_1,
    CCNxTestrigSuiteTest_RequestTest_Errors_1,
    CCNxTestrigSuiteTest_RequestTest_Restrictions_1,

    CCNxTestrigSuiteTest_ContentObjectTest_1,
    CCNxTestrigSuiteTest_ContentObjectTest_2,
    CCNxTestrigSuiteTest_ContentObjectTest_3,
    CCNxTestrigSuiteTest_ContentObjectTest_4,

    CCNxTestrigSuiteTest_LastEntry
} CCNxTestrigSuiteTest;

void ccnxTestrigSuite_RunAll(CCNxTestrig *rig);
CCNxTestrigSuiteTestResult *ccnxTestrigSuite_RunTest(CCNxTestrig *rig, CCNxTestrigSuiteTest test);

#endif // ccnx_testrig_suite_h
