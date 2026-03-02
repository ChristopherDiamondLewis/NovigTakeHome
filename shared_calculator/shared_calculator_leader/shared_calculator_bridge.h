#pragma once

#include <shared_calculator_leader.h>

#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>

namespace Calculator {

class shared_calculator_bridge : public sharedcalculator::Leader::Service {
 public:
  explicit shared_calculator_bridge(const std::shared_ptr<Leader> calculatorLeader);
  virtual ~shared_calculator_bridge();

  grpc::Status GetUpdates(
      grpc::ServerContext* context,
      const sharedcalculator::GetUpdatesRequest* request,
      sharedcalculator::GetUpdatesResponse* response) override;

 private:
  std::shared_ptr<Leader> d_calculatorLeader;
};

}  // namespace Calculator