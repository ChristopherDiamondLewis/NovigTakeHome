#include <grpcpp/grpcpp.h>
#include <shared_calculator_bridge.h>
#include <shared_calculator_leader.h>

#include <iostream>
#include <thread>

int main() {
  std::cout << "Hello world, from shared_calculator_leader!" << std::endl;

  auto leaderCalculator = std::make_shared<Calculator::Leader>();
  auto gRPCbridge =
      std::make_shared<Calculator::shared_calculator_bridge>(leaderCalculator);

  grpc::ServerBuilder builder;
  builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
  builder.RegisterService(gRPCbridge.get());

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

  std::thread updateLeaderThread(
      [&leaderCalculator]() -> void { leaderCalculator->Run(); });

  server->Wait();
  updateLeaderThread.join();

  return 0;
}
