#pragma once

#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>
#include <shared_calculator_leader.h>

namespace Calculator {

class shared_calculator_bridge : public sharedcalculator::Leader::Service {
 public:
  explicit shared_calculator_bridge(
      const std::shared_ptr<Leader> calculatorLeader);
  virtual ~shared_calculator_bridge() = default;

  grpc::Status GetMostRecentValue(
      grpc::ServerContext* context,
      const sharedcalculator::GetMostRecentValueRequest* request,
      sharedcalculator::GetMostRecentValueResponse* response) override;

  grpc::Status StreamUpdates(
      grpc::ServerContext* context,
      const sharedcalculator::GetUpdatesRequest* request,
      grpc::ServerWriter<sharedcalculator::Event>* writer) override;

 private:
  std::shared_ptr<Leader> d_calculatorLeader;
};

}  // namespace Calculator