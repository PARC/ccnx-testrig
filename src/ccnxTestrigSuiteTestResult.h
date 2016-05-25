#ifndef ccnx_testrig_suitetestresult_h
#define ccnx_testrig_suitetestresult_h

#include "ccnxTestrig_Reporter.h"

struct ccnx_testrig_testresult;
typedef struct ccnx_testrig_testresult CCNxTestrigSuiteTestResult;

CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_CreatePass(char *testCase);
CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_CreateFail(char *testCase, char *reason);
void ccnxTestrigSuiteTestResult_Report(CCNxTestrigSuiteTestResult *result, CCNxTestrigReporter *reporter);

#endif
