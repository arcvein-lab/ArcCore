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
