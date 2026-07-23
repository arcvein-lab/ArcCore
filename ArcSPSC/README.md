# ArcSPSC

ArcSPSC is a minimal, header-only C++20 bounded single-producer
single-consumer queue with compile-time capacity.
Element types must be trivially copyable and default constructible.

## Usage

```cpp
#include <arccore/SpscQueue.hpp>

arccore::SpscQueue<int, 8> queue;
queue.try_push(42);

int value = 0;
queue.try_pop(value);
```

## Configure

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Benchmarks

ArcSPSC includes comparative development benchmarks against Boost.Lockfree,
Rigtorp SPSCQueue, and moodycamel ReaderWriterQueue. Folly is also supported
when an installed package provides the `Folly::folly` CMake target.

The currently available measurements were collected on an uncontrolled macOS
development machine and must not be treated as portable performance claims.

- [macOS development benchmark baseline](docs/benchmarks/MACOS_DEVELOPMENT_BASELINE.md)

A controlled public report will require pinned physical cores, documented CPU
topology, fixed frequency settings, Linux performance counters, and published
raw results.

Benchmarks are optional and resolve their dependencies only when enabled.
Boost must be installed. Rigtorp SPSCQueue and moodycamel ReaderWriterQueue are
fetched at documented release tags when they are not installed:

```sh
cmake -S . -B build-benchmarks \
  -DCMAKE_BUILD_TYPE=Release \
  -DARCSPSC_BUILD_BENCHMARKS=ON
cmake --build build-benchmarks --target arcspsc_benchmarks
./build-benchmarks/benchmarks/arcspsc_benchmarks
```

One shared harness compares ArcSPSC, Boost.Lockfree `spsc_queue`, Rigtorp
`SPSCQueue`, moodycamel `ReaderWriterQueue`, and Folly
`ProducerConsumerQueue` when Folly is available. The single-thread case is a
compiler and code-generation baseline, not a concurrency result. In the
two-thread cases, Google Benchmark reports each completed transfer as one
processed item. Construction, allocation, thread lifecycle, ordered
validation, shutdown, and joins are outside the timed region.

ArcSPSC, Boost, Rigtorp, and Folly expose exactly the requested usable
capacity. Boost and Folly reserve an additional sentinel slot internally.
ReaderWriterQueue guarantees at least the requested capacity but rounds its
block allocation: requests for 256, 1024, and 65536 produce aggregate block
capacities of 511, 2044, and 66430 elements respectively, with usable space
also depending on producer/consumer block positions. Its non-allocating
`try_enqueue` operation is used, so measured pushes cannot grow the queue.
These capacity and object-lifetime differences mean the comparison is not
perfectly equivalent.

On Linux, Folly benchmarks require an installed Folly development package
whose CMake configuration exports `Folly::folly`, including its required
transitive link dependencies. Missing Folly disables only Folly cases.

**Development baseline only. Not suitable for published performance claims.**
Credible results require Linux x86_64, producer and consumer pinning to separate
physical cores, fixed CPU frequency, documented CPU topology, compiler and
flags, repeated runs, and controlled system load.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE).
