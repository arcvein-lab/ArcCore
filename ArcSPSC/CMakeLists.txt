cmake_minimum_required(VERSION 3.20)

project(ArcSPSC VERSION 0.1.0 LANGUAGES CXX)

option(ARCSPSC_BUILD_BENCHMARKS "Build ArcSPSC benchmarks" OFF)

add_library(arccore_spsc INTERFACE)
add_library(ArcCore::SPSC ALIAS arccore_spsc)

target_compile_features(arccore_spsc INTERFACE cxx_std_20)
target_include_directories(
  arccore_spsc
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)

enable_testing()

add_executable(arcspsc_tests tests/SpscQueueTests.cpp)
target_link_libraries(arcspsc_tests PRIVATE ArcCore::SPSC Threads::Threads)

# The test harness uses assert, so keep assertions active in Release builds.
if(MSVC)
  target_compile_options(arcspsc_tests PRIVATE /W4 /permissive- /UNDEBUG)
else()
  target_compile_options(
    arcspsc_tests
    PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -UNDEBUG
  )
endif()

add_test(NAME arcspsc_tests COMMAND arcspsc_tests)

if(ARCSPSC_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
