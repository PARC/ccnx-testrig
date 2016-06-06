#ifndef ccnx_testrig_script_h
#define ccnx_testrig_script_h

#include "ccnxTestrig_SuiteTestResult.h"
#include "ccnxTestrig.h"

#include <ccnx/common/ccnx_Interest.h>
#include <ccnx/common/ccnx_ContentObject.h>

struct ccnx_testrig_script;
typedef struct ccnx_testrig_script CCNxTestrigScript;

struct ccnx_testrig_script_step;
typedef struct ccnx_testrig_script_step CCNxTestrigScriptStep;

CCNxTestrigScript *ccnxTestrigScript_Create(char *testCase);

CCNxTestrigScriptStep *ccnxTestrigScript_AddSendStep(CCNxTestrigLinkID linkId, CCNxTlvDictionary *messageDictionary);
CCNxTestrigScriptStep *ccnxTestrigScript_AddReceiveStep(CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *step, bool assertNull, char *failureMessage);

CCNxTestrigSuiteTestResult *ccnxTestrigScript_Execute(CCNxTestrigScript *script, CCNxTestrig *rig);

#endif
