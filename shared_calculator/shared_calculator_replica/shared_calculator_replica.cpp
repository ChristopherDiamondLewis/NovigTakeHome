#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>
#include <shared_calculator_common.h>
#include <shared_calculator_replica.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

namespace Calculator {

Replica::Replica(std::unique_ptr<sharedcalculator::Leader::Stub> stub)
    : d_currValue(0), d_lastIndexGotten(0), d_stub(std::move(stub)) {}

void Replica::Run() {
  std::cout << "starting to stream updates from leader" << std::endl;

  constexpr int STARTING_BACKOFF_MS = 10;
  constexpr int MAX_BACKOFF_MS = 1000;
  int backoffMs{STARTING_BACKOFF_MS};

  while (true) {
    if (const auto mostRecentValue = GetMostRecentValue()) {
      const auto [leaderValue, leaderIndex] = mostRecentValue.value();
      d_currValue = leaderValue;
      d_lastIndexGotten = leaderIndex;
      std::cout << "Replica initialized to value: " << d_currValue << std::endl;

    } else {
      // Leader unreachable, backoff and retry
      std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
      backoffMs = std::min(backoffMs * 2, MAX_BACKOFF_MS);
      continue;
    }

    grpc::ClientContext context;
    sharedcalculator::GetUpdatesRequest request;
    request.set_from_index(d_lastIndexGotten);

    auto streamOfEvents = d_stub->StreamUpdates(&context, request);

    sharedcalculator::Event event;

    while (streamOfEvents->Read(&event)) {
      if (event.eventindex() != d_lastIndexGotten) {
        std::cout << "Out of order event: expected " << d_lastIndexGotten
                  << " but got " << event.eventindex() << ". Re-syncing..."
                  << std::endl;
        break;  // Break stream, trigger GetMostRecentValue() on next loop
      }

      ApplyEvent({.d_operation = event.operation(),
                  .d_argument = event.argument(),
                  .d_eventIndex = event.eventindex()});

      // reset backoff since we successfully got an event!!!!
      backoffMs = STARTING_BACKOFF_MS;
    }

    Utility::HandleGrpcStatus(streamOfEvents->Finish(),
                              "Error streaming updates from leader");
  }
}

void Replica::ApplyEvent(const Event &event) {
  std::cout << "Applying event: " << event << std::endl;
  d_currValue = Calculator::Utility::ApplyCalculation(event, d_currValue);
  std::cout << "New value : " << d_currValue << std::endl;
  d_lastIndexGotten = event.d_eventIndex + 1;
}

std::optional<std::pair<int64_t, size_t>> Replica::GetMostRecentValue() const {
  grpc::ClientContext context;
  sharedcalculator::GetMostRecentValueResponse response;
  sharedcalculator::GetMostRecentValueRequest request;

  grpc::Status status =
      d_stub->GetMostRecentValue(&context, request, &response);

  if (not status.ok()) {
    Utility::HandleGrpcStatus(status,
                              "Error getting most recent value from leader");
    return std::nullopt;
  }

  return std::make_pair(response.current_value(), response.latest_index());
}

}  // namespace Calculator
