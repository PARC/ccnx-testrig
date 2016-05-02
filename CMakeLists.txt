cmake_minimum_required(VERSION 3.3)
project(ccnxTestrig)
include( CTest )

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CCNX_HOME $ENV{CCNX_HOME})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall")

set(CCNX_LIBRARIES longbow longbow-ansiterm parc ccnx_common ccnx_api_portal ccnx_transport_rta ccnx_api_control ccnx_api_notify)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

set(CCNX_TESTRIG_SOURCES
        src/ccnxTestrig_Link.c
        src/ccnxTestrig.c)

include_directories(${CCNX_HOME}/include)

link_directories(${CCNX_HOME}/lib)

add_executable(ccnxTestrig ${CCNX_TESTRIG_SOURCES})
target_link_libraries(ccnxTestrig ${CCNX_LIBRARIES})
install(TARGETS ccnxTestrig RUNTIME DESTINATION bin)

add_test(EmptyTest, echo "OK")
