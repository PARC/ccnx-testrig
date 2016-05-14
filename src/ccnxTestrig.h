#ifndef ccnx_testrig_h
#define ccnx_testrig_h

#include "ccnxTestrig_Link.h"

struct ccnx_testrig;
typedef struct ccnx_testrig CCNxTestrig;

CCNxTestrigLink *ccnxTestrig_GetLinkA(CCNxTestrig *rig);
CCNxTestrigLink *ccnxTestrig_GetLinkB(CCNxTestrig *rig);
CCNxTestrigLink *ccnxTestrig_GetLinkC(CCNxTestrig *rig);

#endif // ccnx_testrig_h
