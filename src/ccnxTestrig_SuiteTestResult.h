#ifndef ccnx_testrig_suitetestresult_h
#define ccnx_testrig_suitetestresult_h

#include "ccnxTestrig_Reporter.h"

struct ccnx_testrig_testresult;
typedef struct ccnx_testrig_testresult CCNxTestrigSuiteTestResult;

CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_Create(char *testCase);
CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_SetPass(CCNxTestrigSuiteTestResult *testCase);
CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_SetFail(CCNxTestrigSuiteTestResult *testCase, char *reason);

void ccnxTestrigSuiteTestResult_LogPacket(CCNxTestrigSuiteTestResult *testCase, PARCBuffer *packet);
void ccnxTestrigSuiteTestResult_Report(CCNxTestrigSuiteTestResult *result, CCNxTestrigReporter *reporter);

#endif
