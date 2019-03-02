#pragma once

#include <testing/echo.grpc.pb.h>

namespace testing {

struct TestClient {
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<testing::proto::Echo::Stub> stub;

    explicit TestClient(const std::string& server_address);
};

} // namespace testing
