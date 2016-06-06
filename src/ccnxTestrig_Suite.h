#ifndef ccnx_testrig_suite_h
#define ccnx_testrig_suite_h

#include "ccnxTestrig.h"
#include "ccnxTestrig_SuiteTestResult.h"

typedef enum {
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1a,
    CCNxTestrigSuiteTest_FIBTest_BasicInterest_1b,
    CCNxTestrigSuiteTest_ContentObjectTest_1,
    CCNxTestrigSuiteTest_ContentObjectTest_2,
    CCNxTestrigSuiteTest_ContentObjectTest_3,
    CCNxTestrigSuiteTest_ContentObjectTest_4,
    CCNxTestrigSuiteTest_ContentObjectTest_5,
    CCNxTestrigSuiteTest_ContentObjectTest_6,
    CCNxTestrigSuiteTest_ContentObjectErrors_1,
    CCNxTestrigSuiteTest_ContentObjectErrors_2,
    CCNxTestrigSuiteTest_ContentObjectErrors_3,
    CCNxTestrigSuiteTest_ContentObjectRestrictions_1,
    CCNxTestrigSuiteTest_ContentObjectRestrictions_2,
    CCNxTestrigSuiteTest_ContentObjectRestrictions_3,
    CCNxTestrigSuiteTest_ContentObjectRestrictions_4,
    CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_1,
    CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_2,
    CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_3,
    CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_4,
    CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_5,
    CCNxTestrigSuiteTest_ContentObjectRestrictionErrors_6,
    CCNxTestrigSuiteTest_LastEntry
} CCNxTestrigSuiteTest;

// TODO: return a list of (CCNxTestrigSuiteTestResult *)'s
void ccnxTestrigSuite_RunAll(CCNxTestrig *rig);
CCNxTestrigSuiteTestResult *ccnxTestrigSuite_RunTest(CCNxTestrig *rig, CCNxTestrigSuiteTest test);

#endif // ccnx_testrig_suite_h
