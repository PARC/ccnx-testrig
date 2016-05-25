#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "ccnxTestrig_Reporter.h"

struct ccnx_testrig_reporter {
    FILE *fp;
};

static bool
_ccnxTestrigReporter_Destructor(CCNxTestrigReporter **reporterPtr)
{
    CCNxTestrigReporter *reporter = *reporterPtr;
    return true;
}

parcObject_ImplementAcquire(ccnxTestrigReporter, CCNxTestrigReporter);
parcObject_ImplementRelease(ccnxTestrigReporter, CCNxTestrigReporter);

parcObject_Override(
	CCNxTestrigReporter, PARCObject,
	.destructor = (PARCObjectDestructor *) _ccnxTestrigReporter_Destructor);

CCNxTestrigReporter *
ccnxTestrigReporter_Create(FILE *fout)
{
    CCNxTestrigReporter *reporter = parcObject_CreateInstance(CCNxTestrigReporter);
    if (reporter != NULL) {
        reporter->fp = fout;
    }
    return reporter;
}

void
ccnxTestrigReporter_Report(CCNxTestrigReporter *reporter, char *message)
{
    fprintf(reporter->fp, "%s\n", message);
}
