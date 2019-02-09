// ///////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Logan Barnes - All Rights Reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ///////////////////////////////////////////////////////////////////////////////////////
#include <hello/hello.grpc.pb.h>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>

#include <iostream>
#include <thread>

namespace hello {

class HelloService : public hello::proto::Greeter::Service {
public:
    grpc::Status SayHello(grpc::ServerContext* /*context*/,
                          const proto::HelloRequest* request,
                          proto::HelloReply* response) override {

        std::cout << "SayHello thread: " << std::this_thread::get_id() << std::endl;
        std::cout << "Recieved request: " << request->ShortDebugString() << std::endl;

        response->set_message("Hello, " + request->name() + "!");

        std::cout << "Sending response: " << response->ShortDebugString() << std::endl;

        return grpc::Status::OK;
    }
};

} // namespace hello

int main() {

    constexpr const char* server_address = "0.0.0.0:9090";

    std::cout << "Main thread: " << std::this_thread::get_id() << std::endl;

    hello::HelloService service;

    grpc::ServerBuilder builder;
    builder.RegisterService(&service);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    server->Wait();

    return 0;
}