#include <shared_calculator_leader.h>

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <optional>
#include <thread>

namespace Calculator {

Leader::Leader()
    : d_currValue(0),
      d_rng(std::random_device{}()),
      d_opDistribution(0, 3),
      d_argDistribution(1, 100),
      d_eventGenerationMsDistribution(10, 1000) {}

void Leader::Run() {
  while (true) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(d_eventGenerationMsDistribution(d_rng)));
    const auto event = CreateRandomEvent();
    SubmitEvent(event);
    const auto valAndIndex = GetCurrentValueAndIndex();
    std::cout << "Current value: " << valAndIndex.first << std::endl;
  }
}

std::optional<Events> Leader::WaitForUpdatesFromIndex(
    const size_t fromIndex, const std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lock(d_mutex);

  const bool updates_available = d_updates_available_cv.wait_for(
      lock, timeout, [&] { return d_events.size() > fromIndex; });

  if (not updates_available) return std::nullopt;

  return GetUpdatesFromWithLock(fromIndex);
}

Events Leader::GetUpdatesFromWithLock(const size_t fromIndex) const {
  // return events from log starting at fromIndex
  Events updates;
  for (size_t i = fromIndex; i < d_events.size(); i++) {
    updates.push_back(d_events[i]);
  }
  return updates;
}

void Leader::SubmitEvent(Event event) {
  // add event to log and update state
  {
    std::lock_guard<std::mutex> lock(d_mutex);
    event.d_eventIndex = d_events.size();
    d_events.push_back(event);
    ApplyCalculation(event);
  }
  std::cout << "submitting event : " << event << std::endl;

  d_updates_available_cv.notify_all();
}

void Leader::ApplyCalculation(const Event &event) {
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
}

std::pair<int64_t, size_t> Leader::GetCurrentValueAndIndex() const {
  std::lock_guard<std::mutex> lock(d_mutex);
  return {d_currValue, d_events.size()};
}

Event Leader::CreateRandomEvent() {
  Event event;
  event.d_operation = d_supportedOperations[d_opDistribution(d_rng)];
  event.d_argument = d_argDistribution(d_rng);
  return event;
}

}  // namespace Calculator