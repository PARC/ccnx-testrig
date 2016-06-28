/*
 * Copyright (c) 2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ################################################################################
 * #
 * # PATENT NOTICE
 * #
 * # This software is distributed under the BSD 2-clause License (see LICENSE
 * # file).  This BSD License does not make any patent claims and as such, does
 * # not act as a patent grant.  The purpose of this section is for each contributor
 * # to define their intentions with respect to intellectual property.
 * #
 * # Each contributor to this source code is encouraged to state their patent
 * # claims and licensing mechanisms for any contributions made. At the end of
 * # this section contributors may each make their own statements.  Contributor's
 * # claims and grants only apply to the pieces (source code, programs, text,
 * # media, etc) that they have contributed directly to this software.
 * #
 * # There is no guarantee that this section is complete, up to date or accurate. It
 * # is up to the contributors to maintain their portion of this section and up to
 * # the user of the software to verify any claims herein.
 * #
 * # Do not remove this header notification.  The contents of this section must be
 * # present in all distributions of the software.  You may only modify your own
 * # intellectual property statements.  Please provide contact information.
 *
 * - Palo Alto Research Center, Inc
 * This software distribution does not grant any rights to patents owned by Palo
 * Alto Research Center, Inc (PARC). Rights to these patents are available via
 * various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
 * intellectual property used by its contributions to this software. You may
 * contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
 */
#ifndef ccnx_testrig_suite_h
#define ccnx_testrig_suite_h

#include "ccnxTestrig.h"
#include "ccnxTestrig_SuiteTestResult.h"

#include <parc/algol/parc_LinkedList.h>

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

/**
 * Run all of the test cases and return the results in a list.
 *
 * Each element of the resultant list will be of type `CCNxTestrigSuiteTestResult`.
 *
 * @param [in] rig The `CCNxTestrig` to use for the tests.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrig *rig = ...
 *
 *     PARCLinkedList *list = ccnxTestrigSuite_RunAll(rig);
 * }
 * @endcode
 */
PARCLinkedList *ccnxTestrigSuite_RunAll(CCNxTestrig *rig);

/**
 * Run a single test case and return the result.
 *
 * @param [in] rig The `CCNxTestrig` to use for the test.
 * @param [in] test The `CCNxTestrigSuiteTest` (enum value) test to run.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrig *rig = ...
 *
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigSuite_RunTest(rig, CCNxTestrigSuiteTest_ContentObjectTest_1);
 * }
 * @endcode
 */
CCNxTestrigSuiteTestResult *ccnxTestrigSuite_RunTest(CCNxTestrig *rig, CCNxTestrigSuiteTest test);
#endif // ccnx_testrig_suite_h
