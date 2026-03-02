cmake_minimum_required(VERSION 3.17)

set(SERVICE "shared_calculator_replica")
set(SERVICE_LIBRARY "shared_calculator_replica_library")

# gRPC/Protobuf dependency (use pkg-config provided by ubuntu package)
find_package(PkgConfig REQUIRED)
find_package(Protobuf REQUIRED)
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
pkg_check_modules(GRPC REQUIRED grpc++)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
include_directories(${GRPC_INCLUDE_DIRS} ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
link_directories(${GRPC_LIBRARY_DIRS} ${PROTOBUF_LIBRARY_DIRS})

file(GLOB_RECURSE ALL_SOURCES SOURCES
    "*.h"
    "*.cpp"
)

# generate C++ and gRPC sources from proto via custom command
set(PROTO_FILE ${CMAKE_SOURCE_DIR}/shared_calculator/proto/sharedCalculator.proto)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(PROTO_SRCS ${GENERATED_DIR}/sharedCalculator.pb.cc)
set(PROTO_HDRS ${GENERATED_DIR}/sharedCalculator.pb.h)
set(GRPC_SRCS ${GENERATED_DIR}/sharedCalculator.grpc.pb.cc)
set(GRPC_HDRS ${GENERATED_DIR}/sharedCalculator.grpc.pb.h)

add_custom_command(
    OUTPUT ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS}
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --grpc_out=${GENERATED_DIR} --cpp_out=${GENERATED_DIR}
         -I ${CMAKE_SOURCE_DIR}/shared_calculator/proto
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_EXECUTABLE}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC sources from ${PROTO_FILE}"
)

add_custom_target(proto_gen_replica ALL
    DEPENDS ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})
add_library(${SERVICE_LIBRARY} ${ALL_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS} ${GRPC_SRCS} ${GRPC_HDRS})

# Make the local headers visible to targets built from this directory so
# includes like <workerFunctions.h> resolve during compilation.
target_include_directories(${SERVICE_LIBRARY} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


add_executable(${SERVICE}
    shared_calculator_replica.m.cpp
)

target_link_libraries(${SERVICE}
    PRIVATE ${SERVICE_LIBRARY}
            ${GRPC_LIBRARIES}
            ${PROTOBUF_LIBRARIES}
)