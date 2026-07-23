# ArcSPSC macOS development benchmark baseline

> [!CAUTION]
> These results are a development baseline collected on an unpinned macOS
> system with uncontrolled CPU frequency and background activity.
>
> They are intended for implementation analysis and experiment comparison.
> They must not be interpreted as portable latency claims or evidence that one
> queue is universally faster than another.

## Environment

| Field | Value |
|---|---|
| ArcCore commit | `a77589961d987d9a93dab83ab0150bf8cba39ceb` |
| CPU | Apple M1, 8 cores (4 performance and 4 efficiency) |
| Architecture | arm64 |
| Operating system | macOS 15.0, build 24A335 |
| Compiler | AppleClang 16.0.0 (`clang-1600.0.26.6`) |
| Build type | Release |
| CMake version | 3.28.3 |
| Google Benchmark version | 1.9.5 |
| Thread pinning | No |
| CPU frequency control | No |
| Background-load isolation | No |

Five repetitions were run for each case with a 100 ms minimum benchmark time.
All 72 available cases completed, and the ArcSPSC correctness test passed.
Folly was not included because no installed package exported `Folly::folly`.

## Implementations included

- ArcSPSC
- Boost.Lockfree `spsc_queue`
- Rigtorp `SPSCQueue` v1.1
- moodycamel `ReaderWriterQueue` v1.0.7

## SingleThread results

Median time per successful push/pop pair. Lower values indicate less measured
time for this workload. Parentheses show wall-time coefficient of variation
(CV).

| Payload | Capacity | ArcSPSC | Boost | Rigtorp | Moodycamel |
|---|---:|---:|---:|---:|---:|
| 8B | 256 | 17.1 ns (0.52%) | 17.7 ns (0.81%) | 12.9 ns (2.57%) | 5.76 ns (6.84%) |
| 8B | 1024 | 17.0 ns (0.22%) | 18.9 ns (9.35%) | 12.9 ns (0.28%) | 5.38 ns (1.44%) |
| 8B | 65536 | 17.2 ns (1.24%) | 18.2 ns (1.11%) | 13.0 ns (9.81%) | 5.42 ns (3.58%) |
| 64B | 256 | 19.8 ns (0.70%) | 20.0 ns (3.09%) | 15.6 ns (4.25%) | 6.87 ns (2.70%) |
| 64B | 1024 | 19.9 ns (0.86%) | 20.0 ns (2.10%) | 14.9 ns (0.81%) | 7.21 ns (7.70%) |
| 64B | 65536 | 20.9 ns (4.83%) | 20.8 ns (0.92%) | 15.7 ns (3.60%) | 7.20 ns (7.50%) |
| 256B | 256 | 31.0 ns (10.71%) | 30.1 ns (9.45%) | 23.4 ns (1.48%) | 18.3 ns (2.29%) |
| 256B | 1024 | 31.7 ns (33.24%) | 28.2 ns (6.10%) | 26.1 ns (0.65%) | 18.5 ns (15.54%) |
| 256B | 65536 | 31.2 ns (1.17%) | 29.1 ns (1.36%) | 27.2 ns (4.62%) | 18.5 ns (8.92%) |

### SingleThread observations

- In this run, ArcSPSC recorded slightly lower medians than Boost for most 8B
  and 64B cases.
- In this run, Boost recorded slightly lower medians for the 256B cases.
- On this development machine, Rigtorp recorded lower medians than ArcSPSC in
  every SingleThread case.
- Moodycamel recorded the lowest SingleThread medians in this run, but this
  workload is a compiler and code-generation baseline, not concurrent queue
  latency.
- ArcSPSC's 256B/1024 result had 33.24% CV and should be treated as unstable.
- Google Benchmark loop control and `DoNotOptimize` are material at these
  nanosecond durations.

## TwoThread results

Median completed-message throughput. Higher values indicate more completed
messages per second for this workload. Parentheses show throughput CV.

| Payload | Capacity | ArcSPSC | Boost | Rigtorp | Moodycamel |
|---|---:|---:|---:|---:|---:|
| 8B | 256 | 29.64M/s (0.74%) | 17.98M/s (8.35%) | 28.40M/s (6.17%) | 75.45M/s (3.25%) |
| 8B | 1024 | 16.63M/s (13.88%) | 18.14M/s (0.86%) | 19.32M/s (6.31%) | 80.23M/s (7.53%) |
| 8B | 65536 | 15.25M/s (22.17%) | 17.83M/s (21.16%) | 15.67M/s (23.81%) | 80.32M/s (31.53%) |
| 64B | 256 | 28.53M/s (5.79%) | 11.27M/s (16.59%) | 27.71M/s (2.60%) | 63.98M/s (8.88%) |
| 64B | 1024 | 27.95M/s (5.15%) | 9.46M/s (11.02%) | 27.42M/s (2.40%) | 62.73M/s (9.25%) |
| 64B | 65536 | 27.73M/s (5.92%) | 6.87M/s (0.76%) | 24.60M/s (12.28%) | 62.77M/s (16.63%) |
| 256B | 256 | 14.65M/s (14.42%) | 5.28M/s (37.27%) | 10.98M/s (6.94%) | 27.14M/s (13.07%) |
| 256B | 1024 | 18.16M/s (1.55%) | 13.91M/s (17.76%) | 12.00M/s (5.03%) | 18.11M/s (43.08%) |
| 256B | 65536 | 17.86M/s (1.67%) | 13.68M/s (7.20%) | 12.06M/s (5.86%) | 26.24M/s (9.40%) |

## Matched comparison

### ArcSPSC and Boost

In this run, ArcSPSC recorded the higher TwoThread median in seven of nine
matched cases:

- ArcSPSC recorded higher medians across every 64B case.
- ArcSPSC recorded higher medians across every 256B case.
- ArcSPSC recorded a higher median at 8B/256.
- Boost recorded higher medians at 8B/1024 and 8B/65536.

Several Boost cases had substantial variability, particularly:

- 256B/256: 37.27%
- 8B/65536: 21.16%
- 256B/1024: 17.76%
- 64B/256: 16.59%

Within the observed variability, these cases do not establish stable margins.

### ArcSPSC and Rigtorp

In this run, ArcSPSC recorded the higher TwoThread median in seven of nine
matched cases:

- ArcSPSC recorded a slightly higher median at 8B/256.
- Rigtorp recorded a higher median at 8B/1024.
- The 8B/65536 results are inconclusive because both CVs exceeded 22%.
- ArcSPSC recorded slightly higher medians for 64B/256 and 64B/1024.
- ArcSPSC recorded higher medians for 64B/65536 and all measured 256B
  capacities.

This run does not show a universal Rigtorp TwoThread advantage. It shows
workload-dependent ordering and significant macOS variability. Rigtorp did
record lower SingleThread medians than ArcSPSC throughout this run.

### ArcSPSC and moodycamel

Moodycamel recorded the higher TwoThread median in eight of nine cases in this
run. At 256B/1024 the medians were nearly identical:

- ArcSPSC: 18.16M/s
- Moodycamel: 18.11M/s

Moodycamel's 43.08% CV makes that case inconclusive.

This is not a capacity-equivalent comparison. ReaderWriterQueue rounds and
overprovisions its internal blocks:

| Requested capacity | Aggregate block capacity |
|---:|---:|
| 256 | 511 |
| 1024 | 2044 |
| 65536 | 66430 |

Its adapter uses non-allocating `try_enqueue`, so growth is not measured, but
the larger effective queue and different block architecture materially affect
cache behavior and backpressure.

## Variability audit

Cases with CV below 5% were generally more stable during this run. The
following TwoThread cases exceeded 10%:

- ArcSPSC 8B/1024: 13.88%
- ArcSPSC 8B/65536: 22.17%
- ArcSPSC 256B/256: 14.42%
- Boost 8B/65536: 21.16%
- Boost 64B/256: 16.59%
- Boost 64B/1024: 11.02%
- Boost 256B/256: 37.27%
- Boost 256B/1024: 17.76%
- Rigtorp 8B/65536: 23.81%
- Rigtorp 64B/65536: 12.28%
- Moodycamel 8B/65536: 31.53%
- Moodycamel 64B/65536: 16.63%
- Moodycamel 256B/256: 13.07%
- Moodycamel 256B/1024: 43.08%

On this development machine, capacity-65536 results were particularly
sensitive to scheduler placement, producer lead, cache footprint, and system
load.

## Fairness and measurement integrity

All implementations used identical:

- Payload definitions
- Requested capacities
- SingleThread loop
- TwoThread producer/consumer loop
- Message counts
- Five repetitions
- Minimum benchmark time
- Real-time mode for TwoThread
- Completed-message accounting

Queue construction, allocation, worker creation, preflight validation,
shutdown, and joins were excluded from timing.

Failed retries were not counted as completed messages. Each timed TwoThread
iteration completed exactly 65,536 transfers.

Material semantic differences remain:

- ArcSPSC uses exact, inline, compile-time storage.
- Boost uses exact usable capacity with an internal sentinel.
- Rigtorp dynamically allocates exact usable capacity during construction.
- Moodycamel allocates rounded blocks and provides at least the requested
  capacity.
- Payload construction and destruction semantics differ between
  implementations.

## Raw data

The raw Google Benchmark JSON or CSV output used to generate these tables
should be stored alongside this report when available.

Raw benchmark output was not retained for this run.

## Conclusion

For this development run:

- ArcSPSC compared favorably with Boost in most concurrent cases.
- ArcSPSC and Rigtorp were close for several 8B and 64B cases, with
  workload-dependent ordering.
- ArcSPSC recorded higher medians than Rigtorp in the measured 256B concurrent
  cases.
- Moodycamel recorded substantially higher medians in most cases, but its
  capacity and internal architecture are not directly equivalent.
- Several concurrent results were too variable for firm conclusions.

A credible public comparison requires Linux x86_64, pinned physical cores,
fixed CPU frequency, documented topology and compiler flags, controlled load,
and published raw results.
