// ///////////////////////////////////////////////////////////////////////////////////////
//                                                                           |________|
//  Copyright (c) 2019 CloudNC Ltd - All Rights Reserved                        |  |
//                                                                              |__|
//        ____                                                                .  ||
//       / __ \                                                               .`~||$$$$
//      | /  \ \         /$$$$$$  /$$                           /$$ /$$   /$$  /$$$$$$$
//      \ \ \ \ \       /$$__  $$| $$                          | $$| $$$ | $$ /$$__  $$
//    / / /  \ \ \     | $$  \__/| $$  /$$$$$$  /$$   /$$  /$$$$$$$| $$$$| $$| $$  \__/
//   / / /    \ \__    | $$      | $$ /$$__  $$| $$  | $$ /$$__  $$| $$ $$ $$| $$
//  / / /      \__ \   | $$      | $$| $$  \ $$| $$  | $$| $$  | $$| $$  $$$$| $$
// | | / ________ \ \  | $$    $$| $$| $$  | $$| $$  | $$| $$  | $$| $$\  $$$| $$    $$
//  \ \_/ ________/ /  |  $$$$$$/| $$|  $$$$$$/|  $$$$$$/|  $$$$$$$| $$ \  $$|  $$$$$$/
//   \___/ ________/    \______/ |__/ \______/  \______/  \_______/|__/  \__/ \______/
//
// ///////////////////////////////////////////////////////////////////////////////////////
#include "test_service.h"

namespace testing {

grpc::Status TestService::operator()(const proto::EchoRequest& request, proto::EchoResponse* response) const {
    response->set_message(request.message());
    response->set_response_number(1);
    return grpc::Status::OK;
}

void TestService::operator()(const proto::EchoRequest& request,
                             net::ServerToClientStream<testing::proto::EchoResponse>* stream) const {
    proto::EchoResponse response{};
    for (int i = 0; i < request.expected_responses(); ++i) {
        response.set_message(request.message());
        response.set_response_number(i);
        stream->write(response);
    }
    stream->finish(grpc::Status::OK);
}

} // namespace testing
