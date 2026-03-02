# simple container that builds the leader service using cmake
FROM ubuntu:22.04

# install build tools
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake build-essential protobuf-compiler-grpc libgrpc++-dev
        

WORKDIR /shared_calculator_replica
# copy whole project so subdirs can build libraries
COPY ./shared_calculator_replica .

# build just the replica service
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j4