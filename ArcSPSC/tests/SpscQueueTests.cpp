#include <arccore/SpscQueue.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <type_traits>
#include <utility>

namespace {

using ContractQueue = arccore::SpscQueue<unsigned, 8>;

static_assert(ContractQueue::capacity() == 8);
static_assert(noexcept(std::declval<ContractQueue&>().try_push(
    std::declval<const unsigned&>())));
static_assert(noexcept(
    std::declval<ContractQueue&>().try_pop(std::declval<unsigned&>())));
static_assert(noexcept(std::declval<const ContractQueue&>().empty()));
static_assert(!std::is_copy_constructible_v<ContractQueue>);
static_assert(!std::is_copy_assignable_v<ContractQueue>);
static_assert(!std::is_move_constructible_v<ContractQueue>);
static_assert(!std::is_move_assignable_v<ContractQueue>);

struct Payload {
  std::uint32_t sequence;
  std::uint16_t kind;
  bool active;
};

static_assert(std::is_trivially_copyable_v<Payload>);
static_assert(std::is_default_constructible_v<Payload>);

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

void test_capacity_boundaries() {
  arccore::SpscQueue<unsigned, 2> queue;

  assert(queue.capacity() == 2);
  assert(queue.try_push(10U));
  assert(queue.try_push(20U));
  assert(!queue.try_push(30U));

  unsigned value = 0;
  assert(queue.try_pop(value));
  assert(value == 10U);
  assert(queue.try_push(30U));

  assert(queue.try_pop(value));
  assert(value == 20U);
  assert(queue.try_pop(value));
  assert(value == 30U);
  assert(!queue.try_pop(value));
}

void test_repeated_wraparound() {
  arccore::SpscQueue<unsigned, 4> queue;
  unsigned next_to_push = 0;
  unsigned next_to_pop = 0;

  for (unsigned cycle = 0; cycle < 1000; ++cycle) {
    for (std::size_t count = 0; count < queue.capacity(); ++count) {
      assert(queue.try_push(next_to_push));
      ++next_to_push;
    }
    assert(!queue.try_push(next_to_push));

    for (std::size_t count = 0; count < queue.capacity(); ++count) {
      unsigned value = 0;
      assert(queue.try_pop(value));
      assert(value == next_to_pop);
      ++next_to_pop;
    }
  }

  assert(next_to_pop == next_to_push);
  assert(queue.empty());
}

void test_interleaved_operations() {
  arccore::SpscQueue<unsigned, 4> queue;
  unsigned value = 0;

  assert(queue.try_push(1U));
  assert(queue.try_push(2U));
  assert(queue.try_push(3U));
  assert(queue.try_pop(value));
  assert(value == 1U);
  assert(queue.try_push(4U));
  assert(queue.try_push(5U));
  assert(!queue.try_push(6U));

  assert(queue.try_pop(value));
  assert(value == 2U);
  assert(queue.try_pop(value));
  assert(value == 3U);
  assert(queue.try_push(6U));
  assert(queue.try_push(7U));

  for (const unsigned expected : {4U, 5U, 6U, 7U}) {
    assert(queue.try_pop(value));
    assert(value == expected);
  }

  assert(queue.empty());
}

void test_struct_payload() {
  arccore::SpscQueue<Payload, 2> queue;
  const Payload input{42U, 7U, true};

  assert(queue.try_push(input));

  Payload output{};
  assert(queue.try_pop(output));
  assert(output.sequence == input.sequence);
  assert(output.kind == input.kind);
  assert(output.active == input.active);
}

void test_concurrent_transfer() {
  constexpr std::uint32_t transfer_count = 1'000'000;
  arccore::SpscQueue<std::uint32_t, 1024> queue;
  std::uint32_t received_count = 0;

  std::thread producer([&queue] {
    for (std::uint32_t value = 0; value < transfer_count; ++value) {
      while (!queue.try_push(value)) {
        std::this_thread::yield();
      }
    }
  });

  std::thread consumer([&queue, &received_count] {
    while (received_count < transfer_count) {
      std::uint32_t value = 0;
      while (!queue.try_pop(value)) {
        std::this_thread::yield();
      }
      assert(value == received_count);
      ++received_count;
    }
  });

  producer.join();
  consumer.join();
  assert(received_count == transfer_count);
  assert(queue.empty());
}

} // namespace

int main() {
  test_new_queue();
  test_push_then_pop();
  test_capacity_boundaries();
  test_repeated_wraparound();
  test_interleaved_operations();
  test_struct_payload();
  test_concurrent_transfer();
}
