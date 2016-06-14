#!/bin/sh
#
# Copyright (c) 2015, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL XEROX OR PARC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ################################################################################
# #
# # PATENT NOTICE
# #
# # This software is distributed under the BSD 2-clause License (see LICENSE
# # file).  This BSD License does not make any patent claims and as such, does
# # not act as a patent grant.  The purpose of this section is for each contributor
# # to define their intentions with respect to intellectual property.
# #
# # Each contributor to this source code is encouraged to state their patent
# # claims and licensing mechanisms for any contributions made. At the end of
# # this section contributors may each make their own statements.  Contributor's
# # claims and grants only apply to the pieces (source code, programs, text,
# # media, etc) that they have contributed directly to this software.
# #
# # There is no guarantee that this section is complete, up to date or accurate. It
# # is up to the contributors to maintain their portion of this section and up to
# # the user of the software to verify any claims herein.
# #
# # Do not remove this header notification.  The contents of this section must be
# # present in all distributions of the software.  You may only modify your own
# # intellectual property statements.  Please provide contact information.
#
# - Palo Alto Research Center, Inc
# This software distribution does not grant any rights to patents owned by Palo
# Alto Research Center, Inc (PARC). Rights to these patents are available via
# various mechanisms. As of January 2016 PARC has committed to FRAND licensing any
# intellectual property used by its contributions to this software. You may
# contact PARC at cipo@parc.com for more information or visit http://www.ccnx.org
#
# @author Kevin Fox, Christopher A. Wood, Palo Alto Research Center (PARC)
# @copyright (c) 2016, Xerox Corporation (Xerox) and Palo Alto Research Center, Inc (PARC).  All rights reserved.
#

#
# URI to service, routes are set on each forwarder instance to forward this URI to its neighbor
#
URI=ccnx:/ccnx/tutorial

#
# Start first forwarder on 9695 then create 20 forwarder instances in a ring
#
PORT=9695

#
# Logging and credential information
#
LOGLEVEL=debug
ATHENA_LOGFILE=athena.out
KEYFILE=keyfile
PASSWORD=foo

#
# Set NOTLOCAL to "local=true" to turn off hoplimit counting
# "local=true" will cause non-terminating forwarding loops in the ring if a service isn't answering
#
NOTLOCAL="local=false"

#####
#####

#
# Default locations for applications that we depend upon
#
CCNX_HOME_BIN=${CCNX_HOME}/bin
ATHENA=${CCNX_HOME_BIN}/athena
ATHENACTL=${CCNX_HOME_BIN}/athenactl
PARC_PUBLICKEY=${CCNX_HOME_BIN}/parc-publickey

DEPENDENCIES="ATHENA ATHENACTL PARC_PUBLICKEY"

#
# Check any set CCNX_HOME/PARC_HOME environment settings, then the default path, then our build directories.
#
for program in ${DEPENDENCIES}
do
    eval defaultPath=\${${program}}               # <NAME>=<DEFAULT_PATH>
    appName=`expr ${defaultPath} : '.*/\(.*\)'`
    if [ -x ${CCNX_HOME}/bin/${appName} ]; then   # check CCNX_HOME
        eval ${program}=${CCNX_HOME}/bin/${appName}
    elif [ -x ${PARC_HOME}/bin/${appName} ]; then # check PARC_HOME
        eval ${program}=${PARC_HOME}/bin/${appName}
    else                                          # check PATH
        eval ${program}=""
        localPathLookup=`which ${appName}`
        if [ $? -eq 0 ]; then
            eval ${program}=${localPathLookup}
        else                                      # use default build directory location
            [ -f ${defaultPath} ] && eval ${program}=${defaultPath}
        fi
    fi
    eval using=\${${program}}
    if [ "${using}" = "" ]; then
        echo Couldn\'t locate ${appName}, set CCNX_HOME or PARC_HOME to its location.
        exit 1
    fi
    echo Using ${program}=${using}
done

ATHENACTL="${ATHENACTL} -p ${PASSWORD} -f ${KEYFILE}"

# Set up a key for athenactl
${PARC_PUBLICKEY} -c ${KEYFILE} ${PASSWORD} athena 1024 365

# Start the forwarder under test.
${ATHENA} -c tcp://localhost:${PORT}/listener 2>&1 > ${ATHENA_LOGFILE} &
trap "kill -HUP $!" INT

# Define the A, B, C port values.
PORTA=$((PORT+1))
PORTB=$((PORT+2))
PORTC=$((PORT+3))

# Connect to the test rig.
${ATHENACTL} -a tcp://localhost:${PORT} add link tcp://`uname -n`:${PORTA}/${NOTLOCAL}/name=athena${PORTA}
echo "Created link A"
read line
${ATHENACTL} -a tcp://localhost:${PORT} add link tcp://`uname -n`:${PORTB}/${NOTLOCAL}/name=athena${PORTB}
echo "Created link B"
read line
${ATHENACTL} -a tcp://localhost:${PORT} add link tcp://`uname -n`:${PORTC}/${NOTLOCAL}/name=athena${PORTC}
echo "Created link C"
read line
${ATHENACTL} -a tcp://localhost:${PORT} set level ${LOGLEVEL}

echo "Start the test rig and connect to the forwarder"
read line
echo "Configuring the FIB routes"

# Create the necessary routes.
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTA} ccnx:/test/a
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTB} ccnx:/test/b
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTC} ccnx:/test/c

${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTA} ccnx:/test/ab
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTB} ccnx:/test/ab

${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTA} ccnx:/test/ac
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTC} ccnx:/test/ac

${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTB} ccnx:/test/bc
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTC} ccnx:/test/bc

${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTA} ccnx:/test/abc
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTB} ccnx:/test/abc
${ATHENACTL} -a tcp://localhost:${PORT} add route athena${PORTC} ccnx:/test/abc

# TODO: run the test rig
wait
