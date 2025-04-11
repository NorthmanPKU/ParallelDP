cmake_minimum_required(VERSION 3.16)
project(ParallelDP)

set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm@14/bin/clang++)
set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/src/include)

add_library(common STATIC
    src/lis.cpp
    src/lcs.cpp
    src/glws.cpp
    src/tmp.cpp
)
target_include_directories(common PUBLIC ${CMAKE_SOURCE_DIR}/src/include)

enable_testing()

add_executable(test_lis EXCLUDE_FROM_ALL
    tests/test_lis.cpp
)
target_link_libraries(test_lis common)
add_test(NAME test_lis COMMAND test_lis)

add_executable(test_lcs EXCLUDE_FROM_ALL
    tests/test_lcs.cpp
)
target_link_libraries(test_lcs common)
add_test(NAME test_lcs COMMAND test_lcs)