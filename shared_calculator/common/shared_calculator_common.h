#pragma once

#include <grpc++/grpc++.h>

#include <cmath>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace Calculator {

struct Event {
  std::string d_operation;
  int64_t d_argument{0};
  size_t d_eventIndex{0};

  friend std::ostream& operator<<(std::ostream& os, const Event& event) {
    if (event.d_operation == "DIVIDE" && event.d_argument == 0) {
      os << event.d_operation << " " << event.d_argument
         << " (division by zero)";
    } else if (event.d_operation == "POWER_OF_TWO" or
               event.d_operation == "SQUARE_ROOT") {
      os << event.d_operation;
    } else {
      os << event.d_operation << " " << event.d_argument;
    }
    return os;
  }
};

using Events = std::vector<Event>;

namespace Utility {
int64_t ApplyCalculation(const Event& event, int64_t currentValue);
void HandleGrpcStatus(const grpc::Status& status,
                      const std::string& userMessage = "");
}  // namespace Utility

}  // namespace Calculator
