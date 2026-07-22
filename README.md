# ArcCore

ArcCore currently contains one self-contained project: [ArcSPSC](ArcSPSC/README.md),
a C++20 bounded single-producer single-consumer queue.

To configure it independently:

```sh
cd ArcSPSC
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
