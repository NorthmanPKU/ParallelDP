cmake_minimum_required(VERSION 3.16)
project(ParallelDP)

set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm@14/bin/clang++)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(lis
    src/lis.cpp
)