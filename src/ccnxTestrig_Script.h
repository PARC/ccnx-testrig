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

/**
 * Create an empty test script for the given test case.
 *
 * @param [in] testCase The name of the test case.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigScript *script = ccnxTestrigScript_Create("special test case");
 * }
 * @endcode
 */
CCNxTestrigScript *ccnxTestrigScript_Create(char *testCase);

/**
 * Add a "send step" to the test case. When executed, this will send the specified
 * packet to the specified link.
 *
 * @param [in] script A `CCNxTestrigScript` instance.
 * @param [in] linkId The link to which the packet should be sent.
 * @param [in] packet The `CCNxTlvDictionary` to send.
 *
 * @return The `CCNxTestrigScriptStep` instance that refers to this step.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigScript *script = ccnxTestrigScript_Create("special test case");
 *
 *     CCNxTlvDictionary *packet = ...
 *     CCNxTestrigScriptStep *sendStep = ccnxTestrigScript_AddSendStep(script, CCNxTestrigLinkID_LinkA, packet);
 * }
 * @endcode
 */
CCNxTestrigScriptStep *ccnxTestrigScript_AddSendStep(CCNxTestrigScript *script, CCNxTestrigLinkID linkId, CCNxTlvDictionary *packet);

/**
 * Add a "receive step" to the test case. When executed, this step will receive a packet
 * from the specified link and verify that it matches that which was sent in the
 * corresponding send step. If not, the failure message is used for the execution result.
 *
 * @param [in] script A `CCNxTestrigScript` instance.
 * @param [in] linkId The link to which the packet should be sent.
 * @param [in] step The referncing `CCNxTestrigScriptStep` step.
 * @param [in] failureMessage The failure message to use when the received packet does not match that which was sent.
 *
 * @return The `CCNxTestrigScriptStep` instance that refers to this step.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigScript *script = ccnxTestrigScript_Create("special test case");
 *
 *     CCNxTlvDictionary *packet = ...
 *     CCNxTestrigScriptStep *sendStep = ccnxTestrigScript_AddSendStep(script, CCNxTestrigLinkID_LinkA, packet);
 *
 *     CCNxTestrigScriptStep *receiveStep = ccnxTestrigScript_AddReceiveStep(script, CCNxTestrigLinkID_LinkB, sendStep, "failed to get the message");
 * }
 * @endcode
 */
CCNxTestrigScriptStep *ccnxTestrigScript_AddReceiveStep(CCNxTestrigScript *script, CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *step, char *failureMessage);

/**
 * Add a "null receive step" to the test case. When executed, this step will
 * attempt to receive a packet from the specified link and verify that it failed.
 *
 * @param [in] script A `CCNxTestrigScript` instance.
 * @param [in] linkId The link to which the packet should be sent.
 * @param [in] step The referncing `CCNxTestrigScriptStep` step.
 * @param [in] failureMessage The failure message to use when the received packet does not match that which was sent.
 *
 * @return The `CCNxTestrigScriptStep` instance that refers to this step.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigScript *script = ccnxTestrigScript_Create("special test case");
 *
 *     CCNxTlvDictionary *packet = ...
 *     CCNxTestrigScriptStep *sendStep = ccnxTestrigScript_AddSendStep(script, CCNxTestrigLinkID_LinkA, packet);
 *
 *     CCNxTestrigScriptStep *failedReceiveStep = ccnxTestrigScript_AddNullReceiveStep(script, CCNxTestrigLinkID_LinkB, sendStep, "failed to get the message");
 * }
 * @endcode
 */
CCNxTestrigScriptStep *ccnxTestrigScript_AddNullReceiveStep(CCNxTestrigScript *script, CCNxTestrigLinkID linkId, CCNxTestrigScriptStep *step, char *failureMessage);

/**
 * Execute the test script and return the result.
 *
 * @param [in] script A `CCNxTestrigScript` instance.
 * @param [in] rig A `CCNxTestrig` instance.
 *
 * @return The `CCNxTestrigSuiteTestResult` result.
 *
 * Example:
 * @code
 * {
 *     CCNxTestrigScript *script = ccnxTestrigScript_Create("special test case");
 *     ... // add steps
 *
 *     CCNxTestrig *rig = ...
 *     CCNxTestrigSuiteTestResult *result = ccnxTestrigScript_Execute(script, rig);
 * }
 * @endcode
 */
CCNxTestrigSuiteTestResult *ccnxTestrigScript_Execute(CCNxTestrigScript *script, CCNxTestrig *rig);
#endif
