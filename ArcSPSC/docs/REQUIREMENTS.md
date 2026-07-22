# Requirements

`arccore::SpscQueue<T, Capacity>` is a C++20 header-only bounded queue for one
producer thread and one consumer thread.

- `Capacity` is a compile-time power of two and is at least two.
- `T` is trivially copyable and default constructible.
- The queue stores exactly `Capacity` elements in internal static storage and
  performs no dynamic allocation.
- The queue is default-constructible, non-copyable, and non-movable.
- `try_push(const T&) noexcept` returns `false` when full.
- `try_pop(T&) noexcept` returns `false` when empty.
- Successful transfers preserve FIFO order.
- `capacity() noexcept` returns `Capacity`.
- `empty() const noexcept` reports whether its atomic position snapshot is
  empty.
- One producer owns all calls to `try_push`; one consumer owns all calls to
  `try_pop`.
