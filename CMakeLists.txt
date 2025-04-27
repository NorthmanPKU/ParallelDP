cmake_minimum_required(VERSION 3.16)
project(ParallelDP VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optional: Enable OpenMP when building OpenCilk
set(OPENCILK_ENABLE_OPENMP ON)

add_library(OpenCilkOptions INTERFACE)
# --- Find OpenMP ---
find_path(OpenMP_INCLUDE_DIR omp.h
    PATHS
        /usr/include
        /usr/local/include
        /usr/lib/gcc/x86_64-redhat-linux/8/include  # 根据您的系统路径调整
)

if(OpenMP_INCLUDE_DIR)
    message(STATUS "Found OpenMP headers at: ${OpenMP_INCLUDE_DIR}")
    include_directories(${OpenMP_INCLUDE_DIR})
    
    target_include_directories(OpenCilkOptions INTERFACE ${OpenMP_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Could not find OpenMP headers (omp.h)")
endif()

set(OpenMP_CXX_FLAGS "-fopenmp")
set(OpenMP_CXX_LIB_NAMES "gomp")  # 如果你使用的是 GNU OpenMP，可以改为 libomp
set(OpenMP_gomp_LIBRARY "/usr/lib/gcc/x86_64-redhat-linux/8/libgomp.so")  # 根据你系统的 libgomp.so 路径调整

find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    message(STATUS "Found OpenMP and added to flags")
else()
    message(FATAL_ERROR "OpenMP not found, please ensure it is installed properly.")
endif()



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
    message(FATAL_ERROR "OpenCilk not found. Please run install.sh.")
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()

# Include your own project headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


# --- Add OpenCilk options ---  
target_compile_options(OpenCilkOptions INTERFACE -fopencilk)
target_include_directories(OpenCilkOptions INTERFACE ${OPENCILK_INSTALL_DIR}/include)
target_link_directories(OpenCilkOptions INTERFACE ${OPENCILK_INSTALL_DIR}/lib)


# Executables
add_executable(lis tests/test_lis.cpp)
add_executable(lcs tests/test_lcs.cpp)
add_executable(glws tests/test_glws.cpp)
add_executable(dsl tests/test_dsl.cpp)

target_link_libraries(lis PRIVATE
    OpenCilkOptions
    OpenMP::OpenMP_CXX
)

target_link_libraries(lcs PRIVATE
    OpenCilkOptions
    OpenMP::OpenMP_CXX
)

target_link_libraries(dsl PRIVATE
    OpenCilkOptions
    OpenMP::OpenMP_CXX
)


# --- Code formatting ---
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
