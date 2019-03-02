#include "test_client.hpp"

#include <grpcpp/create_channel.h>

namespace testing {

TestClient::TestClient(const std::string& server_address)
    : channel(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()))
    , stub(testing::proto::Echo::NewStub(channel)) {}

} // namespace testing
