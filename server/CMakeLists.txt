cmake_minimum_required(VERSION 3.12)
project(hello-proto LANGUAGES CXX)

option(HELLO_USE_DEV_FLAGS "Compile with all the flags" OFF)
option(HELLO_BUILD_TESTS "Use doctest to build unit tests" OFF)

#############################
### Project Configuration ###
#############################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS) # silly microsoft
    set(SYSTEM_FLAGS /experimental:external /external:anglebrackets /external:W0)
endif ()

if (HELLO_USE_DEV_FLAGS)
    if (NOT MSVC)
        set(HELLO_COMPILE_FLAGS -Wall -Wextra -Werror -Wpedantic -Winit-self
                -Wold-style-cast -Woverloaded-virtual -Wsign-conversion
                -Wmissing-declarations -Wmissing-include-dirs -Wshadow -Wno-unknown-pragmas)
    else ()
        set(HELLO_COMPILE_FLAGS /WX)
    endif ()
endif ()

# "Glob is terrible/root of all evil" yeah yeah. CONFIGURE_DEPENDS in cmake 3.12
# helps to fix that and it is super useful when refactoring
cmake_policy(SET CMP0009 NEW)

##################
### Clang-Tidy ###
##################
find_program(CLANG_TIDY_EXE
        NAMES "clang-tidy"
        DOC "Path to clang-tidy executable"
        )
if (NOT CLANG_TIDY_EXE)
    message(STATUS "clang-tidy not found.")
else ()
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-format-style=file" "-fix" "-fix-errors")
endif ()

###########################
### Protobuf Generation ###
###########################
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
add_subdirectory(protos)

##################
### ProtoScene ###
##################
file(GLOB_RECURSE HELLO_SERVER_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/src/net/*
        )

add_executable(hello_server src/exec/hello_server.cpp ${HELLO_SERVER_SOURCE_FILES})
target_link_libraries(hello_server PRIVATE hello_protos)
target_include_directories(hello_server PRIVATE src)

target_compile_options(hello_server PUBLIC ${HELLO_COMPILE_FLAGS})
set_target_properties(hello_server PROPERTIES
        CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

###############
### Testing ###
###############
if (${HELLO_BUILD_TESTS})
    include(FetchContent)

    ### DocTest ###
    FetchContent_Declare(doctest_dl
            GIT_REPOSITORY https://github.com/onqtam/doctest.git
            GIT_TAG 2.2.3
            )

    FetchContent_GetProperties(doctest_dl)
    if (NOT doctest_dl_POPULATED)
        FetchContent_Populate(doctest_dl)
    endif ()

    set(AUTO_GENERATION_MESSAGE "***This file is auto generated. If you modify it the changes will be overwritten.***")
    configure_file(src/testing/hello_tests.hpp.in ${CMAKE_BINARY_DIR}/gen/testing/hello_tests.hpp)

    add_executable(hello_tests src/testing/hello_tests.cpp ${HELLO_SERVER_SOURCE_FILES})
    target_link_libraries(hello_tests PRIVATE hello_protos)
    target_include_directories(hello_tests PRIVATE
            src
            ${doctest_dl_SOURCE_DIR}
            ${CMAKE_BINARY_DIR}/gen
            )

    target_compile_options(hello_tests PUBLIC ${HELLO_COMPILE_FLAGS})
    set_target_properties(hello_tests PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            )
endif ()
