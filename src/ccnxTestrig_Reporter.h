#ifndef ccnxTestrigReporter_h_
#define ccnxTestrigReporter_h_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <parc/algol/parc_HashMap.h>

struct ccnx_testrig_reporter;
typedef struct ccnx_testrig_reporter CCNxTestrigReporter;

CCNxTestrigReporter *ccnxTestrigReporter_Create(FILE *fout);

void ccnxTestrigReporter_Report(CCNxTestrigReporter *reporter, char *message);

#endif // ccnxTestrigReporter
