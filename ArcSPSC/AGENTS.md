# ArcSPSC contributor guidance

## Scope

Work in this directory as an independent C++20 project. Keep the implementation
and documentation limited to the behavior in `docs/REQUIREMENTS.md`.

## Implementation discipline

- Keep the library header-only and dependency-free.
- Preserve the public CMake targets `arccore_spsc` and `ArcCore::SPSC`.
- Preserve the single-producer single-consumer ownership model.
- Do not add APIs or abstractions outside the current requirements.
- Keep memory-ordering decisions documented in `docs/DESIGN.md`.

## Validation

Configure from this directory with `cmake -S . -B build`, build with
`cmake --build build`, and run `ctest --test-dir build --output-on-failure`.
