#include <arccore/SpscQueue.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <thread>

namespace {

void test_new_queue() {
  arccore::SpscQueue<int, 8> queue;
  assert(queue.empty());
  assert(queue.capacity() == 8);
}

void test_push_then_pop() {
  arccore::SpscQueue<int, 8> queue;
  int value = 0;

  assert(queue.try_push(42));
  assert(!queue.empty());
  assert(queue.try_pop(value));
  assert(value == 42);
  assert(queue.empty());
}

void test_fifo_full_and_empty() {
  arccore::SpscQueue<int, 4> queue;

  assert(queue.try_push(10));
  assert(queue.try_push(20));
  assert(queue.try_push(30));
  assert(queue.try_push(40));
  assert(!queue.try_push(50));

  for (const int expected : {10, 20, 30, 40}) {
    int value = 0;
    assert(queue.try_pop(value));
    assert(value == expected);
  }

  int value = 0;
  assert(!queue.try_pop(value));
}

void test_wraparound() {
  arccore::SpscQueue<std::size_t, 4> queue;

  for (std::size_t cycle = 0; cycle < 100; ++cycle) {
    for (std::size_t offset = 0; offset < queue.capacity(); ++offset) {
      assert(queue.try_push(cycle * queue.capacity() + offset));
    }
    for (std::size_t offset = 0; offset < queue.capacity(); ++offset) {
      std::size_t value = 0;
      assert(queue.try_pop(value));
      assert(value == cycle * queue.capacity() + offset);
    }
  }
}

void test_repeated_push_pop() {
  arccore::SpscQueue<std::uint32_t, 2> queue;

  for (std::uint32_t expected = 0; expected < 10000; ++expected) {
    assert(queue.try_push(expected));
    std::uint32_t value = 0;
    assert(queue.try_pop(value));
    assert(value == expected);
  }
}

void test_concurrent_transfer() {
  constexpr std::uint32_t transfer_count = 1'000'000;
  arccore::SpscQueue<std::uint32_t, 1024> queue;

  std::thread producer([&queue] {
    for (std::uint32_t value = 0; value < transfer_count; ++value) {
      while (!queue.try_push(value)) {
        std::this_thread::yield();
      }
    }
  });

  std::thread consumer([&queue] {
    for (std::uint32_t expected = 0; expected < transfer_count; ++expected) {
      std::uint32_t value = 0;
      while (!queue.try_pop(value)) {
        std::this_thread::yield();
      }
      assert(value == expected);
    }
  });

  producer.join();
  consumer.join();
  assert(queue.empty());
}

} // namespace

int main() {
  test_new_queue();
  test_push_then_pop();
  test_fifo_full_and_empty();
  test_wraparound();
  test_repeated_push_pop();
  test_concurrent_transfer();
}
