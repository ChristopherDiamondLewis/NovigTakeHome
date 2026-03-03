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
  /**
   * @brief Initializes the leader with value 0 and empty event history.
   * @note ::Leader() initializes its random event, argument & operation
   * generators.
   */
  Leader();
  ~Leader() = default;

  /**
   * @brief Runs the leader, continuously generating random events, applying
   * them to its state, and adding them to the event log.
   */
  void Run();

  /**
   * @brief Blocks until new events are available after the given index or
   * timeout expires.
   * @param fromIndex Start fetching events after this index (0 = all events)
   * @param timeout Maximum time to wait
   * @return std::optional<std::vector<Event>>, or std::nullopt on timeout
   */
  std::optional<Events> WaitForUpdatesFromIndex(
      const size_t fromIndex, const std::chrono::milliseconds timeout);

  /**
   * @brief Returns the current value and the index of the most recent event.
   * @return Pair of (current_value, latest_event_index)
   */
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