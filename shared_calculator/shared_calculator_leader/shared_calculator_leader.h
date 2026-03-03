#pragma once

#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>

#include <condition_variable>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace Calculator {

struct Event {
  std::string d_operation;
  int64_t d_argument{0};
  size_t d_eventIndex{0};

  friend std::ostream& operator<<(std::ostream& os, const Event& event) {
    os << event.d_operation << " " << event.d_argument;
    return os;
  }
};

using Events = std::vector<Event>;

class Leader {
 public:
  Leader();
  ~Leader() = default;

  void Run();

  std::optional<Events> WaitForUpdatesFromIndex(
      const size_t fromIndex, const std::chrono::milliseconds timeout);
  std::pair<int64_t, size_t> GetCurrentValueAndIndex() const;

 private:
  Events GetUpdatesFromWithLock(const size_t fromIndex) const;
  void SubmitEvent(Event event);
  Event CreateRandomEvent();
  void ApplyCalculation(const Event& event);

 private:
  int64_t d_currValue;
  Events d_events;
  std::vector<std::string> d_supportedOperations{"ADD", "SUBTRACT", "MULTIPLY",
                                                 "DIVIDE"};
  std::mt19937 d_rng;
  std::uniform_int_distribution<size_t> d_opDistribution;
  std::uniform_int_distribution<int64_t> d_argDistribution;
  std::uniform_int_distribution<int64_t> d_eventGenerationMsDistribution;
  std::condition_variable d_updates_available_cv;
  mutable std::mutex d_mutex;
};
}  // namespace Calculator