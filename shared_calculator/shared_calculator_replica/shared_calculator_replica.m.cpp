#include <grpcpp/grpcpp.h>
#include <shared_calculator_replica.h>

#include <iostream>
#include <thread>

int main() {
  std::string leaderHostName{"shared_calculator_leader:50051"};

  // create gRPC channel and stub to communicate with leader
  auto channel =
      grpc::CreateChannel(leaderHostName, grpc::InsecureChannelCredentials());
  auto stub = sharedcalculator::Leader::NewStub(channel);

  auto replica = std::make_shared<Calculator::Replica>(std::move(stub));

  std::cout << "Starting replica - "
            << (std::hash<std::thread::id>{}(std::this_thread::get_id()) % 1000)
            << std::endl;
  replica->Run();

  return 0;
}
