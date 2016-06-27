# CCNxTestrig

CCNxTestrig is an application to perform functional tests for CCNx forwarders.
It treats the forwarder as a black box and assesses its packet forwarding
functionality for correctness. Tests are prescriptive in nature and are
built using compile-time test scripts.

For example, the following test script creates a random Interest with the
name prefix "ccnx:/test/b", a matching Content Object, and then ensures that
the forwarder-under-test will forward these packets correctly. That is,
the Interest and Content Object will be routed to the correct links.

~~~
CCNxName *testName = _createRandomName("ccnx:/test/b");
CCNxInterest *interest = ccnxInterest_Create(testName, 1000, NULL, NULL);
PARCBuffer *testPayload = parcBuffer_Allocate(1024);
CCNxContentObject *content = ccnxContentObject_CreateWithNameAndPayload(testName, testPayload);

CCNxTestrigScript *script = ccnxTestrigScript_Create(testCaseName);
CCNxTestrigScriptStep *step1 = ccnxTestrigScript_AddSendStep(script, CCNxTestrigLinkID_LinkA, interest);
CCNxTestrigScriptStep *step2 = ccnxTestrigScript_AddReceiveStep(script, CCNxTestrigLinkID_LinkB, step1, "Failed to receive an interest packet from the forwarder.");
CCNxTestrigScriptStep *step3 = ccnxTestrigScript_AddSendStep(script, CCNxTestrigLinkID_LinkB, content);
CCNxTestrigScriptStep *step4 = ccnxTestrigScript_AddReceiveStep(script, CCNxTestrigLinkID_LinkA, step3, "Failed to receive a content packet from the forwarder.");

CCNxTestrigSuiteTestResult *testCaseResult = ccnxTestrigScript_Execute(script, rig);
~~~

# CCNxTestrig design

The CCNxTestrig application is composed of five primary elements:

1. Testrig: The main application that creates and configures links to the forwarder
and then executes (part of or the entirety of) the test suite.

2. Link: An abstraction of a forwarder link connection. Currently, UDP and TCP links are supported.

3. Test suite: A collection of compiled test scripts that are executed.

4. Test script: A set of steps to interact with the forwarder to send and receive packets
on specific links.

5. Reporter: A module that produces the test results.

These modules are composed as follows:

~~~
                      +---------+  uses  +------------+
                      |   link  <--------+ testscript |
                      +----^----+        +------^-----+
                           |                    |
                           |creates             |executes
                           |                    |
+------------+  uses  +----+----+        +------+-----+
|  reporter  <--------+ testrig +--------> testsuite  |
+------------+        +---------+        +------------+
                                configures
~~~

# CCNxTestrig scripts

Test scripts are a prescriptive set of steps that are executed in sequence to send and
receive packets to interact with a forwarder. Scripts are created by providing packets to
send verifying that they are received correctly. For example, a script step might be to
send an Interest to link A attached to the forwarder. To verify receipt, the subsequent
step might be a receive step that expects the packet which was sent to be that which was
received.

Currently, test scripts must be written in C code. A future extension would be to implement
a custom DSL to write these tests and have them compile to their C code equivalents. 
