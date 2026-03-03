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
  // constructor, destructor, and other member functions
  Replica(std::unique_ptr<sharedcalculator::Leader::Stub> stub);
  ~Replica() = default;

  void Run();
  std::optional<std::pair<int64_t, size_t>> GetMostRecentValue() const;
  void applyEvent(const Event event);

 private:
  int64_t d_currValue;
  size_t d_lastIndexGotten;
  std::unique_ptr<sharedcalculator::Leader::Stub> d_stub;
};
}  // namespace Calculator