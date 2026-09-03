# incppbuild

Structure-aware incremental compilation for C++.

`incppbuild` is an experimental C++ build system that aims to avoid unnecessary
rebuilds by comparing normalized program structure instead of relying solely on
file timestamps or raw source hashes. Its intended workflow is to analyze a
project with Clang, determine the impact of a source change, and rebuild only
the translation units whose generated output can actually change.

The project is designed around five stages:

1. Parse C++ translation units and their dependencies with Clang LibTooling.
2. Normalize relevant AST structure, including alpha-renaming of local bindings.
3. Fingerprint functions and aggregate them into translation-unit fingerprints.
4. Traverse dependency and symbol-impact graphs to identify affected units.
5. Recompile affected units, reuse valid cached objects, and relink as needed.

## Status

The repository is a buildable C++20 foundation for the complete system. It
currently includes a prototype parser, structural canonicalization,
self-contained SHA-256 fingerprints, cache metadata writing, and reverse
dependency traversal. The remaining work is tracked in the implementation plan:
Clang-based analysis, persistent cache loading, compilation-database support,
selective compiler invocation, object reuse, linking, and benchmark evaluation.

The current parser and demo are proof-of-concept components, not the final build
engine. In particular, the project does not yet invoke a compiler selectively or
reuse cached object files.

## Build

Requirements: CMake 3.20+ and a C++20 compiler (MSVC, Clang, or GCC).

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

On single-configuration generators, omit `--config Debug` and `-C Debug`.

## Languages

- **C++** - 66.5%
- **Python** - 27.4%
- **CMake** - 3.4%
- **Batchfile** - 2.7%

[View repository details](https://github.com/albymariam/Structure-Aware-Incremental-Compiler?tab=readme-ov-file)

## Layout

- `apps/incppbuild/` — build-tool executable and prototype demonstration.
- `include/incppbuild/` — library headers organized by subsystem.
- `src/` — implementations for AST, normalization, fingerprint, cache, core,
  and dependency subsystems.
- `tests/` — CTest-based unit and integration tests.
- `examples/sample_project/` — small multi-file C++ fixture for experiments and
  end-to-end tests.
- `scripts/legacy/` — original batch/Python demo helpers retained for reference.
- `docs/` — project roadmap and implementation plan.
- `.github/workflows/` — GitHub Actions CI.

Generated files belong in `build/` (ignored by Git). Use `work/` only for local
scratch material and `outputs/` only for deliverables intended to be shared.
Future persistent build metadata belongs in `.incppbuild-cache/`.

## Intended build workflow

Once the build engine is complete, a build will follow this flow:

```text
compile_commands.json + source files
                |
                v
       Clang AST and dependency analysis
                |
                v
      normalized structural fingerprints
                |
                v
   change classification + impact traversal
                |
                v
  reuse valid objects / compile affected units
                |
                v
              link output
```

Formatting-only edits and local renames should preserve fingerprints where safe.
Function-body changes should rebuild their translation unit, while public API or
header-layout changes should also rebuild dependent units.

## Prototype demonstration

The `incppbuild_demo` executable shows the current proof-of-concept pipeline on
`examples/sample_project/`: source extraction, canonicalization, fingerprinting,
and cache metadata generation. It demonstrates change classification; it is not
yet a replacement for CMake, Ninja, or a compiler driver.

## Roadmap

See [the project roadmap](docs/project-roadmap.md) and
[implementation plan](docs/implementation-plan.md).
