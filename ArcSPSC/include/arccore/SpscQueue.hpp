#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace arccore {

template <typename T, std::size_t Capacity>
class SpscQueue {
  static_assert(Capacity >= 2, "SpscQueue capacity must be at least two");
  static_assert((Capacity & (Capacity - 1)) == 0,
                "SpscQueue capacity must be a power of two");
  static_assert(std::is_trivially_copyable_v<T>,
                "SpscQueue requires a trivially copyable element type");
  static_assert(std::is_default_constructible_v<T>,
                "SpscQueue requires a default-constructible element type");

public:
  SpscQueue() = default;
  ~SpscQueue() = default;

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;
  SpscQueue(SpscQueue&&) = delete;
  SpscQueue& operator=(SpscQueue&&) = delete;

  bool try_push(const T& value) noexcept {
    const std::size_t write_position = write_position_.load(std::memory_order_relaxed);
    const std::size_t read_position = read_position_.load(std::memory_order_acquire);

    if (write_position - read_position == Capacity) {
      return false;
    }

    std::memcpy(&storage_[write_position & (Capacity - 1)], &value, sizeof(T));
    write_position_.store(write_position + 1, std::memory_order_release);
    return true;
  }

  bool try_pop(T& value) noexcept {
    const std::size_t read_position = read_position_.load(std::memory_order_relaxed);
    const std::size_t write_position = write_position_.load(std::memory_order_acquire);

    if (write_position == read_position) {
      return false;
    }

    std::memcpy(&value, &storage_[read_position & (Capacity - 1)], sizeof(T));
    read_position_.store(read_position + 1, std::memory_order_release);
    return true;
  }

  static constexpr std::size_t capacity() noexcept { return Capacity; }

  bool empty() const noexcept {
    const std::size_t read_position = read_position_.load(std::memory_order_acquire);
    const std::size_t write_position = write_position_.load(std::memory_order_acquire);
    return write_position == read_position;
  }

private:
  std::array<T, Capacity> storage_;
  alignas(64) std::atomic<std::size_t> write_position_{0};
  alignas(64) std::atomic<std::size_t> read_position_{0};
};

} // namespace arccore
