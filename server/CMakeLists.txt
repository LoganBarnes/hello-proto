cmake_minimum_required(VERSION 3.12)
project(MinecraftServer LANGUAGES CXX)

option(MCS_USE_DEV_FLAGS "Compile with all the flags" OFF)

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
        ${CMAKE_CURRENT_LIST_DIR}/src/*
        )

add_executable(minecraft_server ${MINECRAFT_SERVER_SOURCE_FILES})
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
