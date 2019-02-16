#if 0
// project
#include "mcs/minecraft_server.hpp"

// system
#include <iostream>

int main(int argc, const char* argv[]) {

    std::string server_address = "0.0.0.0:9090";

    if (argc > 1) {
        server_address = argv[1];
    }

    mcs::MinecraftServer server(server_address);

    std::cout << "Server running at " << server_address << std::endl;
    server.run_blocking();

    return 0;
}
#else
#if 1
// project
#include "async_server.hpp"

// generated
#include <minecraft/world.grpc.pb.h>

// third-party
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

// system
#include <iostream>

namespace {

grpc::Status say_hello(const minecraft::HelloRequest& request, minecraft::HelloReply* response) {
    response->set_message("Hello, " + request.name() + "!");
    //    std::cout << "Request:  " << request.ShortDebugString() << std::endl;
    //    std::cout << "Response: " << response->ShortDebugString() << std::endl;
    return grpc::Status::OK;
}

} // namespace

int main() {
    net::AsyncServer<minecraft::World> minecraft_server(/*port=*/9090u);

    minecraft_server.register_unary_rpc(&minecraft::World::AsyncService::RequestSayHello, &say_hello);

    minecraft_server.run();

    return 0;
}
#else
/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>

#include "minecraft/world.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;
using minecraft::HelloReply;
using minecraft::HelloRequest;
using minecraft::World;

class ServerImpl final {
public:
    ~ServerImpl() {
        server_->Shutdown();
        // Always shutdown the completion queue after the server.
        cq_->Shutdown();
    }

    // There is no shutdown handling in this code.
    void run() {
        std::string server_address("0.0.0.0:9090");

        ServerBuilder builder;
        // Listen on the given address without any authentication mechanism.
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        // Register "service_" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *asynchronous* service.
        builder.RegisterService(&service_);
        // Get hold of the completion queue used for the asynchronous communication
        // with the gRPC runtime.
        cq_ = builder.AddCompletionQueue();
        // Finally assemble the server.
        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        // Proceed to the server's main loop.
        handle_rpcs();
    }

private:
    // Class encompasing the state and logic needed to serve a request.
    class CallData {
    public:
        // Take in the "service" instance (in this case representing an asynchronous
        // server) and the completion queue "cq" used for asynchronous communication
        // with the gRPC runtime.
        CallData(World::AsyncService* service, ServerCompletionQueue* cq)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE) {
            // Invoke the serving logic right away.
            proceed();
        }

        void proceed() {
            if (status_ == CREATE) {
                // Make this instance progress to the PROCESS state.
                status_ = PROCESS;

                // As part of the initial CREATE state, we *request* that the system
                // start processing SayHello requests. In this request, "this" acts are
                // the tag uniquely identifying the request (so that different CallData
                // instances can serve different requests concurrently), in this case
                // the memory address of this CallData instance.
                service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_, this);
                std::cout << "Created" << std::endl;
            } else if (status_ == PROCESS) {
                // Spawn a new CallData instance to serve new clients while we process
                // the one for this CallData. The instance will deallocate itself as
                // part of its FINISH state.
                new CallData(service_, cq_);

                // The actual processing.
                std::string prefix("Hello ");
                reply_.set_message(prefix + request_.name());

                // And we are done! Let the gRPC runtime know we've finished, using the
                // memory address of this instance as the uniquely identifying tag for
                // the event.
                status_ = FINISH;
                responder_.Finish(reply_, Status::OK, this);
                std::cout << "Processed" << std::endl;
            } else {
                GPR_ASSERT(status_ == FINISH);
                // Once in the FINISH state, deallocate ourselves (CallData).
                std::cout << "Finished" << std::endl;
                delete this;
            }
        }

    private:
        // The means of communication with the gRPC runtime for an asynchronous
        // server.
        World::AsyncService* service_;
        // The producer-consumer queue where for asynchronous server notifications.
        ServerCompletionQueue* cq_;
        // Context for the rpc, allowing to tweak aspects of it such as the use
        // of compression, authentication, as well as to send metadata back to the
        // client.
        ServerContext ctx_;

        // What we get from the client.
        HelloRequest request_;
        // What we send back to the client.
        HelloReply reply_;

        // The means to get back to the client.
        ServerAsyncResponseWriter<HelloReply> responder_;

        // Let's implement a tiny state machine with the following states.
        enum CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_; // The current serving state.
    };

    // This can be run in multiple threads if needed.
    void handle_rpcs() {
        // Spawn a new CallData instance to serve new clients.
        new CallData(&service_, cq_.get());
        void* tag; // uniquely identifies a request.
        bool ok;
        while (true) {
            // Block waiting to read the next event from the completion queue. The
            // event is uniquely identified by its tag, which in this case is the
            // memory address of a CallData instance.
            // The return value of Next should always be checked. This return value
            // tells us whether there is any kind of event or cq_ is shutting down.
            GPR_ASSERT(cq_->Next(&tag, &ok));
            GPR_ASSERT(ok);
            static_cast<CallData*>(tag)->proceed();
        }
    }

    std::unique_ptr<ServerCompletionQueue> cq_;
    World::AsyncService service_;
    std::unique_ptr<Server> server_;
};

int main() {
    ServerImpl server;
    server.run();

    return 0;
}
#endif
#endif
