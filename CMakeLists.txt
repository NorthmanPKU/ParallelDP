cmake_minimum_required(VERSION 3.16)
project(ParallelDP VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: Enable OpenMP when building OpenCilk
set(OPENCILK_ENABLE_OPENMP ON)

# --- Try to find OpenCilk compiler installed locally ---
include(ExternalProject)

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "Looking for OpenCilk compiler at ${CMAKE_BINARY_DIR}/opencilk/bin")

find_program(OPENCILK_CLANG clang
    PATHS
        ${CMAKE_BINARY_DIR}/opencilk/bin
        # $ENV{HOME}/.local/bin
        # /opt/opencilk/bin
    NO_DEFAULT_PATH
)

message(STATUS "OPENCILK_CLANG: ${OPENCILK_CLANG}")

set(OPENCILK_FOUND FALSE)

if(OPENCILK_CLANG)
    execute_process(
        COMMAND ${OPENCILK_CLANG} --version
        OUTPUT_VARIABLE CLANG_VERSION_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(CLANG_VERSION_OUTPUT MATCHES "OpenCilk")
        set(OPENCILK_FOUND TRUE)
        message(STATUS "Found OpenCilk compiler at ${OPENCILK_CLANG}")
    else()
        message(STATUS "Found clang but it's not OpenCilk: ${OPENCILK_CLANG}")
    endif()
else()
    message(STATUS "No clang found in preferred OpenCilk paths")
endif()

# --- If not found, build OpenCilk from source ---
if(NOT OPENCILK_FOUND)
    # err
    message(FATAL_ERROR "OpenCilk not found. Please run install.sh.")
#     message(STATUS "Installing OpenCilk from source...")

#     # Write patch script to modify OPENCILK_RUNTIMES to include openmp
#     set(PATCH_SCRIPT "${CMAKE_BINARY_DIR}/patch_opencilk_build.cmake")
#     file(WRITE ${PATCH_SCRIPT} "
# set(_BUILD_SCRIPT \"${CMAKE_BINARY_DIR}/opencilk/src/OpenCilk/tools/build\")
# file(READ \"\${_BUILD_SCRIPT}\" _BUILD_CONTENTS)
# if(NOT _BUILD_CONTENTS MATCHES \"cheetah;cilktools;openmp\")
#     string(REPLACE
#         \"OPENCILK_RUNTIMES=\\\"cheetah;cilktools\\\" # Required runtimes\"
#         \"OPENCILK_RUNTIMES=\\\"cheetah;cilktools;openmp\\\" # Required runtimes\"
#         _PATCHED_CONTENTS \"\${_BUILD_CONTENTS}\")
#     file(WRITE \"\${_BUILD_SCRIPT}\" \"\${_PATCHED_CONTENTS}\")
# endif()
# ")

#     ExternalProject_Add(OpenCilk
#         PREFIX ${CMAKE_BINARY_DIR}/opencilk
#         GIT_REPOSITORY https://github.com/OpenCilk/infrastructure
#         GIT_TAG opencilk/v2.1
#         CONFIGURE_COMMAND ""
#         BUILD_COMMAND
#             ${CMAKE_BINARY_DIR}/opencilk/src/OpenCilk/tools/get
#                 ${CMAKE_BINARY_DIR}/opencilk/src/OpenCilk/opencilk &&
#             ${CMAKE_COMMAND} -P ${PATCH_SCRIPT} &&
#             ${CMAKE_BINARY_DIR}/opencilk/src/OpenCilk/tools/build
#                 ${CMAKE_BINARY_DIR}/opencilk/src/OpenCilk/opencilk
#                 ${CMAKE_BINARY_DIR}/opencilk/install
#         INSTALL_COMMAND ""
#         BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/opencilk/install/bin/clang++
#     )

#     set(OPENCILK_INSTALL_DIR "${CMAKE_BINARY_DIR}/opencilk/install")
#     set(OpenCilk_C_COMPILER "${OPENCILK_INSTALL_DIR}/bin/clang")
#     set(OpenCilk_CXX_COMPILER "${OPENCILK_INSTALL_DIR}/bin/clang++")
else()
    get_filename_component(OpenCilk_CXX_DIR ${OPENCILK_CLANG} DIRECTORY)
    get_filename_component(OPENCILK_INSTALL_DIR ${OpenCilk_CXX_DIR} DIRECTORY)
    set(OpenCilk_C_COMPILER ${OPENCILK_CLANG})
    find_program(OpenCilk_CXX_COMPILER clang++ HINTS ${OpenCilk_CXX_DIR})
endif()

include_directories("${OPENCILK_INSTALL_DIR}/include")
link_directories("${OPENCILK_INSTALL_DIR}/lib")

# Add compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -fopencilk")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Include your own project headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Executables
add_executable(lis tests/test_lis.cpp)
add_executable(lcs tests/test_lcs.cpp)

target_compile_options(lis PRIVATE -fopencilk)
target_include_directories(lis PRIVATE ${OPENCILK_INSTALL_DIR}/include)
target_link_directories(lis PRIVATE ${OPENCILK_INSTALL_DIR}/lib)

target_compile_options(lcs PRIVATE -fopencilk)
target_include_directories(lcs PRIVATE ${OPENCILK_INSTALL_DIR}/include)
target_link_directories(lcs PRIVATE ${OPENCILK_INSTALL_DIR}/lib)

# Code formatting
find_program(CLANG_FORMAT "clang-format")
if (CLANG_FORMAT)
    file(GLOB_RECURSE ALL_CXX_SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/include/*.h"
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT}
        -i -style=file ${ALL_CXX_SOURCE_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format on source files")
endif()
