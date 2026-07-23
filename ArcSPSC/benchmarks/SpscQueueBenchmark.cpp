#include <arccore/SpscQueue.hpp>

#include <benchmark/benchmark.h>
#include <boost/lockfree/spsc_queue.hpp>
#ifdef ARCSPSC_HAS_FOLLY
#include <folly/ProducerConsumerQueue.h>
#endif
#include <readerwriterqueue.h>
#include <rigtorp/SPSCQueue.h>

#include <array>
#include <barrier>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>

namespace {

struct Payload8 {
  std::uint64_t sequence = 0;
};

struct Payload64 {
  std::uint64_t sequence = 0;
  std::array<std::byte, 56> padding{};
};

struct Payload256 {
  std::uint64_t sequence = 0;
  std::array<std::byte, 248> padding{};
};

static_assert(sizeof(Payload8) == 8);
static_assert(sizeof(Payload64) == 64);
static_assert(sizeof(Payload256) == 256);
static_assert(std::is_trivially_copyable_v<Payload8>);
static_assert(std::is_trivially_copyable_v<Payload64>);
static_assert(std::is_trivially_copyable_v<Payload256>);

template <typename T, std::size_t Capacity>
class ArcSpscBenchmarkQueue {
public:
  bool try_push(const T& value) noexcept { return queue_.try_push(value); }
  bool try_pop(T& value) noexcept { return queue_.try_pop(value); }
  bool empty() noexcept { return queue_.empty(); }

private:
  arccore::SpscQueue<T, Capacity> queue_;
};

template <typename T, std::size_t Capacity>
class BoostSpscBenchmarkQueue {
public:
  bool try_push(const T& value) { return queue_.push(value); }
  bool try_pop(T& value) { return queue_.pop(value); }
  bool empty() { return queue_.empty(); }

private:
  boost::lockfree::spsc_queue<T, boost::lockfree::capacity<Capacity>> queue_;
};

template <typename T, std::size_t Capacity>
class RigtorpSpscBenchmarkQueue {
public:
  RigtorpSpscBenchmarkQueue() : queue_(Capacity) {}

  bool try_push(const T& value) { return queue_.try_push(value); }

  bool try_pop(T& value) {
    T* front = queue_.front();
    if (front == nullptr) {
      return false;
    }
    value = *front;
    queue_.pop();
    return true;
  }

  bool empty() const noexcept { return queue_.empty(); }

private:
  rigtorp::SPSCQueue<T> queue_;
};

#ifdef ARCSPSC_HAS_FOLLY
template <typename T, std::size_t Capacity>
class FollySpscBenchmarkQueue {
public:
  FollySpscBenchmarkQueue() : queue_(Capacity + 1) {}

  bool try_push(const T& value) { return queue_.write(value); }
  bool try_pop(T& value) { return queue_.read(value); }
  bool empty() const { return queue_.isEmpty(); }

private:
  folly::ProducerConsumerQueue<T> queue_;
};
#endif

template <typename T, std::size_t Capacity>
class MoodycamelSpscBenchmarkQueue {
public:
  MoodycamelSpscBenchmarkQueue() : queue_(Capacity) {}

  bool try_push(const T& value) { return queue_.try_enqueue(value); }
  bool try_pop(T& value) { return queue_.try_dequeue(value); }
  bool empty() const { return queue_.size_approx() == 0; }

private:
  moodycamel::ReaderWriterQueue<T> queue_;
};

template <template <typename, std::size_t> class Queue, typename T, std::size_t Capacity>
void single_thread_push_pop(benchmark::State& state) {
  auto queue = std::make_unique<Queue<T, Capacity>>();
  T input{};
  T output{};

  for (auto _ : state) {
    static_cast<void>(_);
    static_cast<void>(queue->try_push(input));
    static_cast<void>(queue->try_pop(output));
    benchmark::DoNotOptimize(output);
  }

  state.SetItemsProcessed(state.iterations());
}

enum class TransferCommand { validate, measure, stop };

template <template <typename, std::size_t> class Queue, typename T, std::size_t Capacity>
void two_thread_throughput(benchmark::State& state) {
  constexpr std::uint64_t validation_messages = 4096;
  constexpr std::uint64_t messages_per_iteration = 65536;

  auto iteration = state.begin();
  auto end = state.end();
  state.PauseTiming();

  auto queue = std::make_unique<Queue<T, Capacity>>();
  std::barrier phase_barrier(3);
  TransferCommand command = TransferCommand::validate;
  std::uint64_t message_count = validation_messages;
  bool validation_succeeded = true;

  std::thread producer([&] {
    T value{};
    for (;;) {
      phase_barrier.arrive_and_wait();
      if (command == TransferCommand::stop) {
        break;
      }

      const std::uint64_t count = message_count;
      for (std::uint64_t sequence = 0; sequence < count; ++sequence) {
        value.sequence = sequence;
        while (!queue->try_push(value)) {
        }
      }
      phase_barrier.arrive_and_wait();
    }
  });

  std::thread consumer([&] {
    T value{};
    for (;;) {
      phase_barrier.arrive_and_wait();
      if (command == TransferCommand::stop) {
        break;
      }

      const std::uint64_t count = message_count;
      if (command == TransferCommand::validate) {
        for (std::uint64_t expected = 0; expected < count; ++expected) {
          while (!queue->try_pop(value)) {
          }
          if (value.sequence != expected) {
            validation_succeeded = false;
          }
        }
      } else {
        for (std::uint64_t received = 0; received < count; ++received) {
          while (!queue->try_pop(value)) {
          }
        }
      }
      phase_barrier.arrive_and_wait();
    }
  });

  phase_barrier.arrive_and_wait();
  phase_barrier.arrive_and_wait();
  if (!validation_succeeded || !queue->empty()) {
    state.SkipWithError("ordered transfer preflight failed");
    command = TransferCommand::stop;
    phase_barrier.arrive_and_wait();
    producer.join();
    consumer.join();
    return;
  }

  command = TransferCommand::measure;
  message_count = messages_per_iteration;

  state.ResumeTiming();
  while (iteration != end) {
    phase_barrier.arrive_and_wait();
    phase_barrier.arrive_and_wait();
    ++iteration;
  }

  command = TransferCommand::stop;
  phase_barrier.arrive_and_wait();
  producer.join();
  consumer.join();

  state.SetItemsProcessed(state.iterations() *
                          static_cast<std::int64_t>(messages_per_iteration));
}

template <template <typename, std::size_t> class Queue, typename T, std::size_t Capacity>
void register_capacity(const char* implementation, const char* payload_size) {
  const std::string prefix = std::string(implementation) + '/';
  const std::string suffix = std::string("/") + payload_size + '/' +
                             std::to_string(Capacity);

  benchmark::RegisterBenchmark(
      (prefix + "SingleThread" + suffix).c_str(),
      &single_thread_push_pop<Queue, T, Capacity>);
  benchmark::RegisterBenchmark(
      (prefix + "TwoThread" + suffix).c_str(),
      &two_thread_throughput<Queue, T, Capacity>)
      ->UseRealTime();
}

template <template <typename, std::size_t> class Queue, typename T>
void register_payload(const char* implementation, const char* payload_size) {
  register_capacity<Queue, T, 256>(implementation, payload_size);
  register_capacity<Queue, T, 1024>(implementation, payload_size);
  register_capacity<Queue, T, 65536>(implementation, payload_size);
}

[[maybe_unused]] const bool benchmarks_registered = [] {
  register_payload<ArcSpscBenchmarkQueue, Payload8>("ArcSPSC", "8B");
  register_payload<ArcSpscBenchmarkQueue, Payload64>("ArcSPSC", "64B");
  register_payload<ArcSpscBenchmarkQueue, Payload256>("ArcSPSC", "256B");
  register_payload<BoostSpscBenchmarkQueue, Payload8>("BoostSPSC", "8B");
  register_payload<BoostSpscBenchmarkQueue, Payload64>("BoostSPSC", "64B");
  register_payload<BoostSpscBenchmarkQueue, Payload256>("BoostSPSC", "256B");
  register_payload<RigtorpSpscBenchmarkQueue, Payload8>("RigtorpSPSC", "8B");
  register_payload<RigtorpSpscBenchmarkQueue, Payload64>("RigtorpSPSC", "64B");
  register_payload<RigtorpSpscBenchmarkQueue, Payload256>("RigtorpSPSC", "256B");
#ifdef ARCSPSC_HAS_FOLLY
  register_payload<FollySpscBenchmarkQueue, Payload8>("FollySPSC", "8B");
  register_payload<FollySpscBenchmarkQueue, Payload64>("FollySPSC", "64B");
  register_payload<FollySpscBenchmarkQueue, Payload256>("FollySPSC", "256B");
#endif
  register_payload<MoodycamelSpscBenchmarkQueue, Payload8>("MoodycamelSPSC", "8B");
  register_payload<MoodycamelSpscBenchmarkQueue, Payload64>("MoodycamelSPSC", "64B");
  register_payload<MoodycamelSpscBenchmarkQueue, Payload256>("MoodycamelSPSC", "256B");
  return true;
}();

} // namespace

BENCHMARK_MAIN();
