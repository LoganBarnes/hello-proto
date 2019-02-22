cmake_minimum_required(VERSION 3.12)
project(MinecraftServer LANGUAGES CXX)

option(MCS_USE_DEV_FLAGS "Compile with all the flags" OFF)
option(MCS_BUILD_TESTS "Use doctest to build unit tests" OFF)

#############################
### Project Configuration ###
#############################
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (MSVC)
    add_definitions(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS) # silly microsoft
    set(SYSTEM_FLAGS /experimental:external /external:anglebrackets /external:W0)
endif ()

if (MCS_USE_DEV_FLAGS)
    if (NOT MSVC)
        set(MCS_COMPILE_FLAGS -Wall -Wextra -Werror -Wpedantic -Winit-self
                -Wold-style-cast -Woverloaded-virtual -Wsign-conversion
                -Wmissing-declarations -Wmissing-include-dirs -Wshadow -Wno-unknown-pragmas)
    else ()
        set(MCS_COMPILE_FLAGS /WX)
    endif ()
endif ()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

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
file(GLOB_RECURSE MINECRAFT_SERVER_SOURCE_FILES
        LIST_DIRECTORIES false
        CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_LIST_DIR}/src/mcs/*
        ${CMAKE_CURRENT_LIST_DIR}/src/net/*
        )

add_executable(minecraft_server
        src/exec/minecraft_server_instance.cpp
        ${MINECRAFT_SERVER_SOURCE_FILES}
        )
target_link_libraries(minecraft_server PRIVATE
        minecraft_protos
        Threads::Threads
        )
target_include_directories(minecraft_server PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

target_compile_options(minecraft_server PUBLIC ${MCS_COMPILE_FLAGS})
set_target_properties(minecraft_server PROPERTIES
        CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )

###############
### Testing ###
###############
if (${MCS_BUILD_TESTS})
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

    ### Testing ###
    file(GLOB_RECURSE TESTING_SOURCE_FILES
            LIST_DIRECTORIES false
            CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_LIST_DIR}/src/testing/*
            )

    add_executable(mcs_tests ${TESTING_SOURCE_FILES} ${MCS_SERVER_SOURCE_FILES})
    target_link_libraries(mcs_tests PRIVATE
            testing_protos
            Threads::Threads
            )
    target_include_directories(mcs_tests PRIVATE
            src
            ${doctest_dl_SOURCE_DIR}
            )
    target_compile_definitions(mcs_tests PRIVATE -DBUILD_WITH_TESTS)

    target_compile_options(mcs_tests PUBLIC ${MCS_COMPILE_FLAGS})
    set_target_properties(mcs_tests PROPERTIES
            CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
            )

    ### Coverage ###
    # add test coverage capabilities
    find_program(LCOV_EXE
            NAMES "lcov"
            DOC "Path to lcov executable"
            )

    if (LCOV_EXE AND CMAKE_COMPILER_IS_GNUCXX AND CMAKE_BUILD_TYPE MATCHES "Debug")
        include(CodeCoverage)
        setup_target_for_coverage(mcs_coverage mcs_tests coverage)
        set(PROFILE_FLAGS --coverage -fprofile-arcs -ftest-coverage -fno-inline)

        target_compile_options(mcs_tests PUBLIC ${PROFILE_FLAGS})
        target_link_libraries(mcs_tests PUBLIC gcov)
    endif ()
endif ()
