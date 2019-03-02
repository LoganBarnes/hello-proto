#pragma once

#include <testing/echo.grpc.pb.h>
#include <net/connections.hpp>

namespace testing {

struct TestService {
    grpc::Status operator()(const testing::proto::EchoRequest& request, testing::proto::EchoResponse* response) const;

    void operator()(const proto::EchoRequest& request,
                    net::ServerToClientStream<testing::proto::EchoResponse>* stream) const;

    std::unordered_set<net::ServerToClientStream<testing::proto::EchoResponse>*> client_connections;
};

} // namespace testing
