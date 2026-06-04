cmake_minimum_required(VERSION 3.31)
project(ZenBitStreamTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------------------
# Optional Sanitizers Configuration Toggle
# -------------------------------------------------------------------
# This adds a configurable option. Default is OFF.
option(ENABLE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)

# Tell CMake to prepare the CTest framework
enable_testing()

# -------------------------------------------------------------------
# Google Test Fetching
# -------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# -------------------------------------------------------------------
# Target Configuration
# -------------------------------------------------------------------
add_executable(run_tests test_bitstream.cpp)

target_include_directories(run_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(run_tests PRIVATE gtest_main)

# -------------------------------------------------------------------
# Compiler Flags & Conditional Sanitizers
# -------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # Base compilation flags applied to everything
    target_compile_options(run_tests PRIVATE -Wall -Wextra -Wpedantic)

    # Check if the user explicitly enabled sanitizers
    if(ENABLE_SANITIZERS)
        message(STATUS "Building with Address and Undefined Behavior Sanitizers enabled.")
        target_compile_options(run_tests PRIVATE 
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=all
        )
        target_link_options(run_tests PRIVATE 
            -fsanitize=address,undefined
        )
    else()
        message(STATUS "Sanitizers are DISABLED.")
    endif()
endif()

# Enable automated discovery of test macros for CTest
include(GoogleTest)
gtest_discover_tests(run_tests)
