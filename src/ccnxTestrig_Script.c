
#include "ccnxTestrig_SuiteTestResult.h"
#include "ccnxTestrig_Script.h"

#include <parc/algol/parc_LinkedList.h>

struct ccnx_testrig_script {
    char *testCase;
};

static bool
_ccnxTestrigScript_Destructor(CCNxTestrigScript **resultPtr)
{
    CCNxTestrigScript *result = *resultPtr;

    parcMemory_Deallocate(&(result->testCase));

    return true;
}

parcObject_ImplementAcquire(ccnxTestrigScript, CCNxTestrigScript);
parcObject_ImplementRelease(ccnxTestrigScript, CCNxTestrigScript);

parcObject_Override(
	CCNxTestrigScript, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigScript_Destructor);

struct ccnx_testrig_script_step {
    int stepIndex;
};

static bool
_ccnxTestrigScriptStep_Destructor(CCNxTestrigScriptStep **resultPtr)
{
    return true;
}

parcObject_ImplementAcquire(ccnxTestrigScriptStep, CCNxTestrigScriptStep);
parcObject_ImplementRelease(ccnxTestrigScriptStep, CCNxTestrigScriptStep);

parcObject_Override(
	CCNxTestrigScriptStep, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigScriptStep_Destructor);


CCNxTestrigScript *
ccnxTestrigScript_Create(char *testCase)
{
    CCNxTestrigScript *result = parcObject_CreateInstance(CCNxTestrigScript);

    if (result != NULL) {
        result->testCase = malloc(strlen(testCase));
        strcpy(result->testCase, testCase);
    }

    return result;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddSendStep(CCNxTestrigLinkID linkId, CCNxTlvDictionary *messageDictionary)
{
    return NULL;
}

CCNxTestrigScriptStep *
ccnxTestrigScript_AddReceiveStep(CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *step, bool assertNull, char *failureMessage)
{
    return NULL;
}

CCNxTestrigSuiteTestResult *
ccnxTestrigScript_Execute(CCNxTestrigScript *script, CCNxTestrig *rig)
{
    return NULL;
}
