find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

# search directory for protobuf
set(PROTOBUF_IMPORT_DIRS ${CMAKE_CURRENT_LIST_DIR})

set(PROTO_SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
set(PROTO_DST_DIR ${CMAKE_BINARY_DIR}/protos)

# make the output directory
file(MAKE_DIRECTORY ${PROTO_DST_DIR})

function(create_proto_library target_name proto_dir)
    # gather proto files
    file(GLOB_RECURSE HELLO_PROTO_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${proto_dir}/*.proto
            )

    # generate service objects
    grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTO_DST_DIR} ${HELLO_PROTO_FILES})

    add_library(${target_name} ${GRPC_HDRS} ${GRPC_SRCS})
    target_link_libraries(${target_name}
            PUBLIC protobuf::libprotobuf
            PUBLIC gRPC::grpc++
            )
    target_include_directories(${target_name} SYSTEM PUBLIC ${PROTO_DST_DIR})
endfunction()

create_proto_library(hello_protos ${PROTO_SRC_DIR}/hello)
