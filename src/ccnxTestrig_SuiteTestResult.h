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
#ifndef ccnx_testrig_suitetestresult_h
#define ccnx_testrig_suitetestresult_h

#include "ccnxTestrig_Reporter.h"

#include <parc/algol/parc_Buffer.h>

struct ccnx_testrig_testresult;
typedef struct ccnx_testrig_testresult CCNxTestrigSuiteTestResult;

/**
 * Create a `CCNxTestrigSuiteTestResult` result container.
 *
 * This stores the context for a test case result and any additional information,
 * such as an error message. Initially, the test result is neither a success
 * or failure.
 *
 * @param [in] testCase An identifier for the test case.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create("special test");
 * }
 * @endcode
 */
CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_Create(char *testCase);

/**
 * Mark the `CCNxTestrigSuiteTestResult` as a successful test case.
 *
 * @param [in] testCase The `CCNxTestrigSuiteTestResult` to be marked as a success.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create("easy test");
 *     ...
 *     ccnxTestrigSuiteTestResult_SetPass(result);
 * }
 * @endcode
 */
CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_SetPass(CCNxTestrigSuiteTestResult *testCase);

/**
 * Mark the `CCNxTestrigSuiteTestResult` as a failed test case.
 *
 * @param [in] testCase The `CCNxTestrigSuiteTestResult` to be marked as a failure.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create("hard test");
 *     ...
 *     ccnxTestrigSuiteTestResult_SetFail(result);
 * }
 * @endcode
 */
CCNxTestrigSuiteTestResult *ccnxTestrigSuiteTestResult_SetFail(CCNxTestrigSuiteTestResult *testCase, char *reason);

/**
 * Determine if the test case is a failure.
 *
 * @param [in] testCase The `CCNxTestrigSuiteTestResult` to be inspected.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create("hard test");
 *     ...
 *     ccnxTestrigSuiteTestResult_SetFail(result);
 *     ...
 *     if (ccnxTestrigSuiteTestResult_IsFailure(result)) {
 *          // handle the failure
 *     }
 * }
 * @endcode
 */
bool ccnxTestrigSuiteTestResult_IsFailure(CCNxTestrigSuiteTestResult *testCase);

/**
 * Log a packet to this test case. This will accumulate the packet so that it can
 * later be retrieved for debugging purposes.
 *
 * @param [in] testCase The `CCNxTestrigSuiteTestResult` to be amended.
 * @param [in] packet A wire-encoded packet in a `PARCBuffer`.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create("hard test");
 *     ...
 *     PARCBuffer *packet = ...
 *     ccnxTestrigSuiteTestResult_LogPacket(result, packet);
 * }
 * @endcode
 */
void ccnxTestrigSuiteTestResult_LogPacket(CCNxTestrigSuiteTestResult *testCase, PARCBuffer *packet);

/**
 * Report a `CCNxTestrigSuiteTestResult` instance.
 *
 * @param [in] testCase The `CCNxTestrigSuiteTestResult` to be reported.
 * @param [in] reporter A `CCNxTestrigReporter` instance.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigSuiteTestResult_Create("hard test");
 *     ...
 *     PARCBuffer *packet = ...
 *     ccnxTestrigSuiteTestResult_LogPacket(result, packet);
 * }
 * @endcode
 */
void ccnxTestrigSuiteTestResult_Report(CCNxTestrigSuiteTestResult *result, CCNxTestrigReporter *reporter);
#endif
