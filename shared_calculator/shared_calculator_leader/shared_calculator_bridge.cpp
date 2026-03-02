#include <shared_calculator_bridge.h>


#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>

namespace Calculator {

shared_calculator_bridge::shared_calculator_bridge(
    const std::shared_ptr<Leader> calculatorLeader)
    : d_calculatorLeader(calculatorLeader) {}

shared_calculator_bridge::~shared_calculator_bridge() = default;

grpc::Status shared_calculator_bridge::GetUpdates(
    grpc::ServerContext* context,
    const sharedcalculator::GetUpdatesRequest* request,
    sharedcalculator::GetUpdatesResponse* response) {

  const auto updates =
      d_calculatorLeader->GetUpdatesFrom(request->from_index());
  for (const auto& update : updates) {
    auto* event = response->add_events();
    event->set_operation(update.d_operation);
    event->set_argument(update.d_argument);
    event->set_eventindex(update.d_eventIndex);
  }

  return grpc::Status::OK;
}

}  // namespace Calculator