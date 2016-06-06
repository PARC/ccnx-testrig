#ifndef ccnx_testrig_h
#define ccnx_testrig_h

#include "ccnxTestrig_Link.h"
#include "ccnxTestrig_Reporter.h"

struct ccnx_testrig;
typedef struct ccnx_testrig CCNxTestrig;

typedef enum {
    CCNxTestrigLinkID_LinkA,
    CCNxTestrigLinkID_LinkB,
    CCNxTestrigLinkID_LinkC
} CCNxTestrigLinkID;

CCNxTestrigReporter *ccnxTestrig_GetReporter(CCNxTestrig *rig);

CCNxTestrigLink *ccnxTestrig_GetLinkByID(CCNxTestrig *rig, CCNxTestrigLinkID linkID);
CCNxTestrigLink *ccnxTestrig_GetLinkA(CCNxTestrig *rig);
CCNxTestrigLink *ccnxTestrig_GetLinkB(CCNxTestrig *rig);
CCNxTestrigLink *ccnxTestrig_GetLinkC(CCNxTestrig *rig);

#endif // ccnx_testrig_h
