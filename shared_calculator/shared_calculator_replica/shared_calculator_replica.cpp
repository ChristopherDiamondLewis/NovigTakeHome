#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>
#include <shared_calculator_replica.h>

#include <chrono>
#include <iostream>
#include <optional>
#include <thread>
#include <vector>

namespace {
void HandleGrpcStatus(const grpc::Status& status,
                      const std::string& userErrorMessage = "") {
  if (not status.ok()) {
    std::cerr << "gRPC error: " << status.error_message()
              << ", error code: " << status.error_code()
              << ", user error message: " << userErrorMessage << std::endl;
  }
}
}  // namespace

namespace Calculator {

Replica::Replica(std::unique_ptr<sharedcalculator::Leader::Stub> stub)
    : d_currValue(0), d_lastIndexGotten(0), d_stub(std::move(stub)) {}

void Replica::Run() {
  std::cout << "starting to stream updates from leader" << std::endl;

  constexpr int STARTING_BACKOFF_MS = 10;
  constexpr int MAX_BACKOFF_MS = 1000;
  int backoffMs{STARTING_BACKOFF_MS};

  while (true) {
    const auto mostRecentValue = GetMostRecentValue();
    if (mostRecentValue.has_value()) {
      const auto [leaderValue, leaderIndex] = mostRecentValue.value();
      d_currValue = leaderValue;
      d_lastIndexGotten = leaderIndex;
      std::cout << "Replica initialized to value: " << d_currValue
                << " at event index: " << d_lastIndexGotten << std::endl;

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

    HandleGrpcStatus(streamOfEvents->Finish(),
                     "Error streaming updates from leader");
  }
}

void Replica::ApplyEvent(const Event event) {
  std::cout << "Applying event: " << event << std::endl;
  ApplyCalculation(event);
  d_lastIndexGotten = event.d_eventIndex + 1;
}

void Replica::ApplyCalculation(const Event& event) {
  // Note: We do not handle int64_t overflow/underflow for ADD, SUBTRACT,
  // MULTIPLY. For production, would add bounds checking or use arbitrary
  // precision arithmetic. Division by zero is handled below.
  if (event.d_operation == "ADD") {
    d_currValue += event.d_argument;
  } else if (event.d_operation == "SUBTRACT") {
    d_currValue -= event.d_argument;
  } else if (event.d_operation == "MULTIPLY") {
    d_currValue *= event.d_argument;
  } else if (event.d_operation == "DIVIDE") {
    if (event.d_argument != 0) {
      d_currValue /= event.d_argument;
    } else {
      std::cerr << "Division by zero attempted, skipping" << std::endl;
    }
  } else {
    std::cerr << "Unknown operation: " << event.d_operation << std::endl;
  }
  std::cout << "Current value: " << d_currValue << std::endl;
}

std::optional<std::pair<int64_t, size_t>> Replica::GetMostRecentValue() const {
  grpc::ClientContext context;
  sharedcalculator::GetMostRecentValueResponse response;
  sharedcalculator::GetMostRecentValueRequest request;

  grpc::Status status =
      d_stub->GetMostRecentValue(&context, request, &response);

  if (not status.ok()) {
    HandleGrpcStatus(status, "Error getting most recent value from leader");
    return std::nullopt;
  }

  return std::make_pair(response.current_value(), response.latest_index());
}

}  // namespace Calculator
