# Change FROM ubuntu:22.04 to include more deps upfront
FROM ubuntu:22.04

# Add missing deps: git, clang (compiler)
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        cmake build-essential protobuf-compiler-grpc libgrpc++-dev libprotobuf-dev pkg-config

# Copy ENTIRE shared_calculator dir (proto, leader, replica all needed)
WORKDIR /replica_app
COPY . .

# build services (currently both leader and replica, but we can split this up if needed)
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j8

