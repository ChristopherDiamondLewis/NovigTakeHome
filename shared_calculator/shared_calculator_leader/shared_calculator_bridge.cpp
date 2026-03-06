#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>
#include <shared_calculator_bridge.h>

namespace Calculator {

std::chrono::milliseconds MAX_TIMEOUT{
    5000};  // ~5 second timeout for waiting for updates

shared_calculator_bridge::shared_calculator_bridge(
    const std::shared_ptr<Leader> calculatorLeader)
    : d_calculatorLeader(calculatorLeader) {}

grpc::Status shared_calculator_bridge::StreamUpdates(
    grpc::ServerContext* context,
    const sharedcalculator::GetUpdatesRequest* request,
    grpc::ServerWriter<sharedcalculator::Event>* writer) {
  size_t fromIndex = request->from_index();

  while (not context->IsCancelled()) {
    const auto updatesFromLeader =
        d_calculatorLeader->WaitForUpdatesFromIndex(fromIndex, MAX_TIMEOUT);

    if (not updatesFromLeader.has_value())
      continue;  // loop again and wait for updates or cancellation

    for (const auto& update : updatesFromLeader.value()) {
      sharedcalculator::Event eventToWrite;
      eventToWrite.set_operation(update.d_operation);
      eventToWrite.set_argument(update.d_argument);
      eventToWrite.set_eventindex(update.d_eventIndex);

      if (not writer->Write(eventToWrite)) {
        return grpc::Status::OK;  // not an error the client just disconnected
      }
    }
    fromIndex = updatesFromLeader.value().back().d_eventIndex +
                1;  // next index to wait for
  }
  return grpc::Status::OK;
}

grpc::Status shared_calculator_bridge::GetMostRecentValue(
    grpc::ServerContext* context,
    const sharedcalculator::GetMostRecentValueRequest* request,
    sharedcalculator::GetMostRecentValueResponse* response) {
  const auto [currentValue, currentIndex] =
      d_calculatorLeader->GetCurrentValueAndIndex();
  response->set_current_value(currentValue);
  response->set_latest_index(currentIndex);

  return grpc::Status::OK;
}

}  // namespace Calculator