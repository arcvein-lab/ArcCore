include(FetchContent)

find_package(Boost QUIET)
if(NOT Boost_FOUND)
  message(
    FATAL_ERROR
    "ArcSPSC benchmarks require an installed Boost package with Boost.Lockfree headers."
  )
endif()

find_package(SPSCQueue 1.1 QUIET CONFIG)
find_path(RIGTORP_SPSCQUEUE_INCLUDE_DIR NAMES rigtorp/SPSCQueue.h)
if(TARGET SPSCQueue::SPSCQueue
   OR TARGET SPSCQueue
   OR RIGTORP_SPSCQUEUE_INCLUDE_DIR)
  message(STATUS "ArcSPSC benchmarks: using installed Rigtorp SPSCQueue")
else()
  FetchContent_Declare(
    rigtorp_spscqueue
    GIT_REPOSITORY https://github.com/rigtorp/SPSCQueue.git
    GIT_TAG v1.1
  )
  FetchContent_MakeAvailable(rigtorp_spscqueue)
  message(STATUS "ArcSPSC benchmarks: fetched Rigtorp SPSCQueue v1.1")
endif()

find_package(readerwriterqueue 1.0.7 QUIET CONFIG)
find_path(
  MOODYCAMEL_READERWRITERQUEUE_INCLUDE_DIR
  NAMES readerwriterqueue.h
  PATH_SUFFIXES readerwriterqueue
)
if(TARGET readerwriterqueue::readerwriterqueue
   OR TARGET readerwriterqueue
   OR MOODYCAMEL_READERWRITERQUEUE_INCLUDE_DIR)
  message(STATUS "ArcSPSC benchmarks: using installed ReaderWriterQueue")
else()
  FetchContent_Declare(
    readerwriterqueue_source
    GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
    GIT_TAG v1.0.7
  )
  FetchContent_MakeAvailable(readerwriterqueue_source)
  message(STATUS "ArcSPSC benchmarks: fetched ReaderWriterQueue v1.0.7")
endif()

find_package(folly QUIET CONFIG)
if(NOT TARGET Folly::folly)
  find_package(Folly QUIET CONFIG)
endif()
if(TARGET Folly::folly)
  set(ARCSPSC_HAS_FOLLY ON)
  message(STATUS "ArcSPSC benchmarks: Folly ProducerConsumerQueue enabled")
else()
  set(ARCSPSC_HAS_FOLLY OFF)
  message(
    STATUS
    "ArcSPSC benchmarks: Folly skipped; install Folly with the Folly::folly CMake target to enable it"
  )
endif()

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG 192ef10025eb2c4cdd392bc502f0c852196baa48 # v1.9.5
)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(arcspsc_benchmarks SpscQueueBenchmark.cpp)
target_link_libraries(arcspsc_benchmarks PRIVATE ArcCore::SPSC benchmark::benchmark)

if(TARGET Boost::headers)
  target_link_libraries(arcspsc_benchmarks PRIVATE Boost::headers)
elseif(TARGET Boost::boost)
  target_link_libraries(arcspsc_benchmarks PRIVATE Boost::boost)
else()
  target_include_directories(arcspsc_benchmarks PRIVATE ${Boost_INCLUDE_DIRS})
endif()

if(TARGET SPSCQueue::SPSCQueue)
  target_link_libraries(arcspsc_benchmarks PRIVATE SPSCQueue::SPSCQueue)
elseif(TARGET SPSCQueue)
  target_link_libraries(arcspsc_benchmarks PRIVATE SPSCQueue)
else()
  target_include_directories(
    arcspsc_benchmarks PRIVATE ${RIGTORP_SPSCQUEUE_INCLUDE_DIR}
  )
endif()

if(TARGET readerwriterqueue::readerwriterqueue)
  target_link_libraries(
    arcspsc_benchmarks PRIVATE readerwriterqueue::readerwriterqueue
  )
elseif(TARGET readerwriterqueue)
  target_link_libraries(arcspsc_benchmarks PRIVATE readerwriterqueue)
else()
  target_include_directories(
    arcspsc_benchmarks PRIVATE ${MOODYCAMEL_READERWRITERQUEUE_INCLUDE_DIR}
  )
endif()

if(ARCSPSC_HAS_FOLLY)
  target_compile_definitions(arcspsc_benchmarks PRIVATE ARCSPSC_HAS_FOLLY=1)
  target_link_libraries(arcspsc_benchmarks PRIVATE Folly::folly)
endif()
