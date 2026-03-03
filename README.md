# Chirs' Distributed Calculator 
Hello all!

My name is Chris and this is my distributed calculator,

You can follow the [How to Run](#how-to-run) and [Manual Testing](#manual-testing) sections to best test its functionality, or if you'd like to get a good understanding of the different trade offs and design choices I made, feel free to read on!

## Overview

Leader generates random calculator operations on shared state. Multiple replicas consume a gRPC event stream and apply the same operations to stay in sync, like a shared document, but with math!

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

## Architecture

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

## Implementation

### Event Stream & Restart Detection

Replica calls `GetMostRecentValue()` before opening stream to sync state and detect restarts.

```cpp
// Replica.Run() main loop
while (true) {
  // Try to sync to leader's current state
  auto mostRecentValue = GetMostRecentValue();
  if (!mostRecentValue.has_value()) {
    // Leader unreachable, backoff and retry
    sleep(backoffMs);
    backoffMs = min(backoffMs * 2, MAX_BACKOFF_MS);
    continue;
  }
  
  auto [leaderValue, leaderIndex] = mostRecentValue.value();
  d_currValue = leaderValue;
  d_lastIndexGotten = leaderIndex;
  
  // Open stream from the synced position
  auto stream = d_stub->StreamUpdates(from_index=d_lastIndexGotten);
  
  // Consume events
  sharedcalculator::Event event;
  while (stream->Read(&event)) {
    if (event.eventindex() != d_lastIndexGotten) {
      // Out of order detected, break and re-sync
      break;
    }
    
    ApplyEvent(event);
    d_lastIndexGotten++;
    backoffMs = STARTING_BACKOFF_MS;  // Reset on successful event
  }
  
  // Stream ended, loop back to GetMostRecentValue()
}
```

If `latest_index` goes backwards, leader restarted. Replica resets to new baseline. 

### Architecture Choices

**gRPC Streaming**: Originally I considered polling the leader repeatedly (replicas asking "do you have updates?"), but that creates a gap: if the leader dies right after a replica checks and then restarts, the replica sleeps waiting for its next polling interval and misses the new events. With streaming, the connection is always open and the leader pushes events the moment it generates them, so nothing gets missed. The continuous push is also more efficient than repeatedly asking "do you have anything for me?"

**Separate Transport Layer**: Keep business logic away from gRPC. Makes the leader easy to test independently and transport swappable.

**Eventual Consistency**: Replicas converge to the leader's state over time. Individual events are applied in order, but there's a window where replicas may differ. This is acceptable for read-heavy workloads where strong consistency isn't required.

### Out of Order Detection

Replica checks `if (event.eventindex != d_lastIndexGotten)` before applying. Mismatch triggers break and re-sync.

## AI & Tools
I used Copilot generally for gRPC (specifically protobuf build issues), boilerplate code for the Dockerfile and docker-compose.yaml, and the initial skeleton for this README.

I also use Copilot for making sweeping changes with the filesystem if it can hold enough context. For example I asked it to change application names all at once to match, and help me get the correct `clang-format` for my `format.sh` script.

Lastly Copilot also helped with considering good timeouts (like exponential backoff min and maxes) and what to test and not test in my unit tests.

## Known Limitations & Outside of Scope

- No persistence (leader crash = data loss)
- No overflow/underflow protection (int64_t wraps silently)
- No metrics/observability (stdout logs only)
- Single leader (no failover)

## Outside Resources Used

- [Eventual Consistency lecture from a Distributed Systems course](https://www.youtube.com/watch?v=9uCP3qHNbWw)
- [Finite State Machines in games](https://www.youtube.com/watch?v=-ZP2Xm-mY4E)
- [Exponential Backoff on Connection Loss](https://en.wikipedia.org/wiki/Exponential_backoff)