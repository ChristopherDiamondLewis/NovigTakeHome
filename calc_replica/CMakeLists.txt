cmake_minimum_required(VERSION 3.17)

set(SERVICE "calc_replica")
set(SERVICE_LIBRARY "calc_replica_library")


file(GLOB_RECURSE ALL_SOURCES SOURCES
    "*.h"
    "*.cpp"
) 

add_library(${SERVICE_LIBRARY} ${ALL_SOURCES})

# Make the local headers visible to targets built from this directory so
# includes like <workerFunctions.h> resolve during compilation.
target_include_directories(${SERVICE_LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


add_executable(${SERVICE}
    calc_replica.m.cpp
)