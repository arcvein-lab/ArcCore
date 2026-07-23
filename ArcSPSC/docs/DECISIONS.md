# Decisions

- The public type is `arccore::SpscQueue<T, Capacity>`.
- C++20 compile-time constraints reject capacities below two, capacities that
  are not powers of two, and element types that are not trivially copyable or
  default constructible.
- One fixed `std::array` holds every slot; all `Capacity` slots are usable.
- Atomic `std::size_t` counters are monotonically increasing and array indexing
  alone uses a power-of-two mask.
- Producer and consumer counters are aligned independently to 64 bytes.
- Publication and slot reuse use acquire/release ordering; owner-only position
  reads use relaxed ordering.
- Byte copying implements noexcept transfer for trivially copyable values.
- The queue supports only default construction and deletes copy and move
  operations.
- Tests use one standalone `assert`-based executable registered with CTest.
- Optional microbenchmarks use Google Benchmark and are excluded from normal
  builds. They cover one-thread push/pop and two-thread transfer for 8-, 64-,
  and 256-byte payloads at capacities 256, 1024, and 65536.
- The first comparison queue is an installed Boost.Lockfree `spsc_queue` using
  the same workloads and user-visible capacities through benchmark-only
  adapters. Boost is never fetched and is required only for benchmark builds.
- Rigtorp SPSCQueue and moodycamel ReaderWriterQueue are benchmark-only
  comparisons. Installed packages or headers are preferred; otherwise CMake
  fetches the official Rigtorp `v1.1` and readerwriterqueue `v1.0.7` tags.
- Folly ProducerConsumerQueue cases are compiled only when an installed Folly
  package exports `Folly::folly`. Folly and its dependency tree are never
  fetched automatically.
- ArcSPSC, Boost, Rigtorp, and Folly adapters request the same usable capacity.
  Folly receives one additional internal slot because its constructor reports
  usable capacity as `size - 1`.
- ReaderWriterQueue is constructed with the requested capacity and uses
  `try_enqueue`, which cannot allocate. Its block rounding provides at least
  the request rather than an exact fixed capacity, so its comparison is
  explicitly non-equivalent in capacity and layout.
- Comparison queues may copy-construct and destroy payloads, while ArcSPSC
  transfers bytes between preconstructed objects. Queue construction,
  allocation, validation, and teardown are excluded from timing.
