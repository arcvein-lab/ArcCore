# Decisions

- The public type is `arccore::SpscQueue<T, Capacity>`.
- C++20 compile-time constraints reject capacities below two, capacities that
  are not powers of two, and element types that are not trivially copyable.
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
