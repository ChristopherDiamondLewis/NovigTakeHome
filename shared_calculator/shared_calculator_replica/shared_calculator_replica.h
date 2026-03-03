#pragma once

#include <shared_calculator_leader.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Calculator {
class Replica {
 public:
  Replica(std::unique_ptr<sharedcalculator::Leader::Stub> stub);
  ~Replica() = default;
  /**
   * @brief Runs the replica, continuously trying to stream updates from the
   * leader and applying them to its local state.
   */
  void Run();

 private:
  /**
   * @brief Fetches the current value and latest event index from the leader.
   *
   * @note I ran into an issue where killing and restarting
   * a replica made it so the replicas value was set **back to 0**
   * and just knowing the next `Event` was not good enough to get the replica
   * back to the correct value
   *
   * @return A pair of (current_value, latest_event_index), or std::nullopt on
   * error.
   */
  std::optional<std::pair<int64_t, size_t>> GetMostRecentValue() const;
  void ApplyEvent(const Event event);
  void ApplyCalculation(const Event& event);

 private:
  int64_t d_currValue;
  size_t d_lastIndexGotten;
  std::unique_ptr<sharedcalculator::Leader::Stub> d_stub;
};
}  // namespace Calculator