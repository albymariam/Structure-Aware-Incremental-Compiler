# Implementation Plan

## Phase 1 — Foundation

- [x] C++20 CMake project and CI.
- [x] Change-level model.
- [x] Reverse dependency graph with breadth-first impact traversal.
- [x] Fingerprint module boundary (temporary deterministic implementation).

## Phase 2 — Structural analysis

- [ ] Add LLVM/Clang discovery in CMake.
- [ ] Implement `RecursiveASTVisitor` extraction of function definitions.
- [ ] Define a normalized AST representation.
- [ ] Alpha-rename parameters and local bindings while preserving symbol scope.

## Phase 3 — Build decisions

- [x] Use the existing self-contained SHA-256 fingerprint implementation.
- [ ] Persist cache metadata as JSON in `.incppbuild-cache/`.
- [ ] Read `compile_commands.json` and include dependencies.
- [ ] Invoke the selected host compiler only for affected units.

## Phase 4 — Evaluation

- [ ] Create reproducible multi-module benchmark fixtures.
- [ ] Record standard-build and `incppbuild` timings for each edit scenario.
- [ ] Publish result tables and limitations.
