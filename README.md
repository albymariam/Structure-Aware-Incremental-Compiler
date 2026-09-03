# incppbuild

Structure-aware incremental compilation for C++.

`incppbuild` is a research prototype that aims to avoid unnecessary rebuilds by
fingerprinting normalized C++ function structure rather than relying solely on
file timestamps or raw source hashes.

## Status

The repository contains a buildable C++20 foundation plus the initial prototype:
regex-based AST extraction, alpha-renaming canonicalization, self-contained SHA-256
fingerprints, JSON-style cache output, dependency-impact graph, command-line entry
point, review demo, and a minimal test suite. Clang LibTooling integration and true
object-cache orchestration are the next implementation milestones.

## Build

Requirements: CMake 3.20+ and a C++20 compiler (MSVC, Clang, or GCC).

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

On single-configuration generators, omit `--config Debug` and `-C Debug`.

## Layout

- `apps/incppbuild/` — executable entry point and CLI handling.
- `apps/incppbuild/demo_main.cpp` — the existing Review 1 demonstration.
- `include/incppbuild/` — public module interfaces.
- `src/` — module implementations.
- `tests/` — fast, standalone unit tests.
- `examples/` — small C++ projects used for experiments.
- `scripts/legacy/` — original batch/Python demo helpers retained for reference.
- `docs/` — design notes, roadmap, and benchmark protocol.
- `.github/workflows/` — GitHub Actions CI.

Generated files belong in `build/` (ignored by Git). Use `work/` only for local
scratch material and `outputs/` only for deliverables intended to be shared.

## Roadmap

See [the project roadmap](docs/project-roadmap.md) and
[implementation plan](docs/implementation-plan.md).
