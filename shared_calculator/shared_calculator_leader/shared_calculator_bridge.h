#pragma once

#include <sharedCalculator.grpc.pb.h>
#include <sharedCalculator.pb.h>
#include <shared_calculator_leader.h>

namespace Calculator {

class shared_calculator_bridge : public sharedcalculator::Leader::Service {
 public:
  /**
   * @brief Creates a gRPC service bridge wrapping the leader.
   * @param calculatorLeader std::shared_ptr<Leader> pointer to the calculator
   * leader.
   */
  explicit shared_calculator_bridge(
      const std::shared_ptr<Leader> calculatorLeader);
  virtual ~shared_calculator_bridge() = default;

  /**
   * @brief gRPC RPC: Returns the current calculator value and latest event
   * index. Used by replicas to detect leader restarts and initialize state.
   */
  grpc::Status GetMostRecentValue(
      grpc::ServerContext* context,
      const sharedcalculator::GetMostRecentValueRequest* request,
      sharedcalculator::GetMostRecentValueResponse* response) override;

  /**
   * @brief gRPC streaming RPC: Continuously sends events from the given index
   * to the client. Replicas use this to consume events in order and stay in
   * sync.
   */
  grpc::Status StreamUpdates(
      grpc::ServerContext* context,
      const sharedcalculator::GetUpdatesRequest* request,
      grpc::ServerWriter<sharedcalculator::Event>* writer) override;

 private:
  std::shared_ptr<Leader> d_calculatorLeader;
};

}  // namespace Calculator