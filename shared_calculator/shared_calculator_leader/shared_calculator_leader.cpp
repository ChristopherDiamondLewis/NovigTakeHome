#include <shared_calculator_leader.h>

#include <chrono>
#include <condition_variable>
#include <optional>
#include <thread>

namespace Calculator {

Leader::Leader() : d_currValue(0){};

void Leader::Run() {
  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    const auto event = CreateRandomEvent();
    SubmitEvent(event);
    const auto valAndIndex = GetCurrentValueAndIndex();
    std::cout << "Current value: " << valAndIndex.first << std::endl;
  }
}

std::optional<Events> Leader::WaitForUpdatesFromIndex(
    const size_t fromIndex, const std::chrono::milliseconds timeout) {
  // wait until there are updates available starting from fromIndex, or until
  // timeout
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
  if (event.d_operation == "ADD") {
    d_currValue += event.d_argument;
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
  event.d_operation = "ADD";
  event.d_argument = 1;
  return event;
}

}  // namespace Calculator