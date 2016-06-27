#!/bin/bash
${CCNX_HOME}/bin/parc-publickey -c test.pkcs12 test test

${CCNX_HOME}/bin/athenactl -f test.pkcs12 -p test add link udp://127.0.0.1:9696/listener/name=link1
${CCNX_HOME}/bin/athenactl -f test.pkcs12 -p test add link udp://127.0.0.1:9697/listener/name=link2
${CCNX_HOME}/bin/athenactl -f test.pkcs12 -p test add link udp://127.0.0.1:9698/name=link3

${CCNX_HOME}/bin/athenactl -f test.pkcs12 -p test add route link3 ccnx:/test

# ./ccnxTestrig -t 0 127.0.0.1 9696
