# Design

## Ring buffer

The queue owns one `std::array<T, Capacity>`. Producer and consumer positions
increase monotonically using unsigned modular arithmetic. Masking with
`Capacity - 1` occurs only when a position indexes the array. The queue is full
when `write_position - read_position == Capacity` and empty when the positions
are equal.

## Index ownership

Only the producer writes the atomic write position, and only the consumer
writes the atomic read position. Each is `alignas(64)` so the two ownership
domains begin on separate cache-line boundaries.

The producer copies a value into a slot, then publishes the new write position
with a release store. The consumer observes it with an acquire load before
reading the slot. The consumer then publishes the new read position with a
release store; the producer uses an acquire load before reusing that slot.
Owner-side position loads are relaxed.

## Benchmark measurement

The optional microbenchmark pauses Google Benchmark timing before queue
construction, allocation, worker creation, readiness synchronization, and
ordered preflight validation. Timing resumes only for steady-state transfer
batches. Shutdown, joins, and final checks occur after timing stops.

All comparison queues use shared workload templates through benchmark-only
adapters. ArcSPSC, Boost.Lockfree, Rigtorp, and Folly expose the requested
usable capacity. Boost and Folly reserve a sentinel slot internally.
moodycamel ReaderWriterQueue allocates rounded blocks that provide at least the
requested capacity; the adapter uses its non-allocating `try_enqueue` API.
Adapters also normalize differing copy-construction, assignment, destruction,
and empty-query APIs without changing the library queue.
