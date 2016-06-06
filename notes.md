Test suite script

Input: test rig (with links) and script

Script: CCNxTestScript
    - series of steps
    - step format:
        - command (send/receive)
        - link (a/b/c)
        - if send: include target (a tlvdictionary)
        - if receive: include point to previous step to compare, i.e., "compare received against target from step #2"
        - expect failure (bool)
        - failure message

... example of building a script
CCNxTestScriptStep *Step1 = ccnxTestScript_AddSendStep(link_id, target message);
CCNxTestScriptStep *Step2 = ccnxTestScript_AddReceiveStep(link_id, Step1, failure flag, "failure message");
...
CCNxTestscriptTestResult *result = ccnxTestScript_Execute(script, testrig);

    - adding a step returns a CCNxTestScriptStep 
        - internally:
            - index into script list
            - identifier for script (so can't mix steps)
        - used when adding ReceiveSteps

TODO:
- write the class for the script
- write a function that implements a script and returns a test result
- remove logic from existing functions
