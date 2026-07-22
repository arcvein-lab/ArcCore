# ArcSPSC

ArcSPSC is a minimal, header-only C++20 bounded single-producer
single-consumer queue with compile-time capacity.
Element types must be trivially copyable and default constructible.

## Usage

```cpp
#include <arccore/SpscQueue.hpp>

arccore::SpscQueue<int, 8> queue;
queue.try_push(42);

int value = 0;
queue.try_pop(value);
```

## Configure

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE).
