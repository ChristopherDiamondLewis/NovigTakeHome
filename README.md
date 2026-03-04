# Chris' Distributed Calculator 
Hello all!

My name is Chris and this is my distributed calculator.

You can follow the [How to Run](#how-to-run) & [Manual Testing](#manual-testing) sections to best test its functionality, or if you'd like to get a good understanding of the different trade offs and design choices I made, feel free to read on!

## Quick Links

- [Overview](#overview)
- [How to Run](#how-to-run)
- [Demo](#demo)
- [Unit Tests](#unit-tests)
- [Manual Testing](#manual-testing)
- [State Machine](#state-machine)
- [Replication Model](#replication-model)
- [AI & Tools](#ai--tools)
- [Known Limitations](#known-limitations--outside-of-scope)
- [Resources](#outside-resources-used)

## Overview

Leader calculator will generate random operations maintaing an in memory data set of `Events`. Multiple replicas consume a gRPC event stream and apply the same operations to stay in sync, like a shared document, but with math and eventual consistency.

## Demo

<div align="center">

[![Watch the demo video](https://img.youtube.com/vi/5Awo9vqemVI/0.jpg)](https://www.youtube.com/watch?v=5Awo9vqemVI)

*[shared calculator demo](https://www.youtube.com/watch?v=5Awo9vqemVI)*

</div>

## How to Run 

```bash
# Assuming you are at the top level of project
docker compose up -dV --force-recreate --build

# Terminal 1
docker compose exec shared_calculator_leader bash
cd start_calculator
./leader.sh

# Terminal 2 (can add more terminals and create more replicas if you want!)
docker compose exec shared_calculator_replica bash
cd start_calculator
./replica.sh
```

Watch logs. All replicas should show the same `Current value: X` eventually.

## Unit tests

I did write some unit tests for my public interfaces. After building, run:

```bash
cd build && ctest --output-on-failure
```

## Manual Testing

After following instructions in [How to Run](#how-to-run), you can do any of the following to see how the shared calculator works.

**Multi-replica convergence**: Add replicas in separate terminals. They all converge within seconds.

**Kill & restart a replica**: It re-syncs from the leader and catches up.

**Kill the leader**: Replicas backoff and keep retrying. Restart the leader; on reconnection they detect the restart via index regression and sync to the new baseline.

## State Machine

Calculator with one `int64_t` value. Operations become immutable events with an index:

```cpp
struct Event {
  string operation;  // "ADD", "SUBTRACT", "MULTIPLY", "DIVIDE", etc.
  int64_t argument;  // 1-100 (random)
  size_t eventIndex; // Index of event within leaders dataset
};
```

Leader will generate a random event every few milliseconds while the replicas subscribe to those events and correctly apply them, synchronizing with the leader.

## Replication Model

```
Leader               gRPC Stream               Replicas
(owns state)                             (read-only copy)
    |                                             |
    +---------Event 1--------------------->     Replica 1
    |
    +---------Event 2--------------------->     Replica 2
    |
    +---------Event N---------------------->    Replica N
```

**Leader**: Owns state, generates events, pushes via gRPC.  
**Replica**: Consumes stream, applies events, re-syncs if leader restarts or out of order detected.

### Key Trade Offs

**gRPC Streaming**: Originally I considered polling the leader repeatedly (replicas asking "do you have updates?"), but that creates a gap: if the leader dies right after a replica checks and then restarts, the replica sleeps waiting for its next polling interval and misses the new events. With streaming, the connection is always open and the leader pushes events the moment it generates them, so nothing gets missed. The continuous push is also more efficient than repeatedly asking "do you have anything for me?"

**Separate Transport Layer**: Keep business logic away from gRPC. Makes the leader easy to test independently and transport swappable.


### Out of Order Detection

Replica checks `if (event.eventindex != d_lastIndexGotten)` before applying. Mismatch triggers break and re-sync.

## AI & Tools
I used Copilot generally for gRPC (specifically protobuf build issues), boilerplate code for the Dockerfile and docker-compose.yaml, and the initial skeleton for this README.

I also use Copilot for making sweeping changes with the filesystem if it can hold enough context. For example I asked it to change application names all at once to match, and help me get the correct `clang-format` for my `format.sh` script.

Lastly Copilot also helped with considering good timeouts (like exponential backoff min/maxes) and what to test and **not** test in my unit tests.

## Known Limitations & Outside of Scope

- No persistence (leader crash = data loss)
- No overflow/underflow protection (int64_t wraps silently)
- No metrics/observability (stdout logs only)
- Single leader (no failover)

## Outside Resources Used

- [Eventual Consistency lecture from a Distributed Systems course](https://www.youtube.com/watch?v=9uCP3qHNbWw)
- [Finite State Machines in games](https://www.youtube.com/watch?v=-ZP2Xm-mY4E)
- [Exponential Backoff on Connection Loss](https://en.wikipedia.org/wiki/Exponential_backoff)
- [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/)
