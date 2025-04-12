# set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm@14/bin/clang++)
# set(CMAKE_CXX_STANDARD 20)

cmake_minimum_required(VERSION 3.16)
project(ParallelDP VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Add compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# # Create library for trees
# add_library(tree_lib STATIC
#     src/tree/segment_tree.cpp
#     # Add other tree implementations if needed
# )

add_library(problem_lib STATIC
    src/dp/lcs.cpp
    # src/dp/lis.cpp
)

# Set properties for libraries
# set_target_properties(tree_lib PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     POSITION_INDEPENDENT_CODE ON
# )

# Link OpenMP to library
# target_link_libraries(tree_lib PUBLIC OpenMP::OpenMP_CXX)

# Create executables for LCS and LIS
add_executable(lcs tests/dp_tests/lcs_test.cpp)
# add_executable(lis src/dp/lis.cpp)

# Link libraries to executables
target_link_libraries(lcs OpenMP::OpenMP_CXX problem_lib)
# target_link_libraries(lis OpenMP::OpenMP_CXX)

# Main executable
# add_executable(parallel_dp_test test.cpp)
# target_link_libraries(parallel_dp_test tree_lib)

# Tests
# enable_testing()

# Tree tests
# add_executable(tree_test tests/tree_tests/tree_test.cpp)
# target_link_libraries(tree_test tree_lib)
# add_test(NAME TreeTest COMMAND tree_test)

# add_executable(segment_tree_test tests/tree_tests/segment_tree.cpp)
# target_link_libraries(segment_tree_test tree_lib)
# add_test(NAME SegmentTreeTest COMMAND segment_tree_test)