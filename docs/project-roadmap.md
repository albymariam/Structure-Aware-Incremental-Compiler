# Project Roadmap

## Goal

Build a native C++ framework that identifies semantic changes using normalized
Clang AST fingerprints and selectively recompiles the affected translation units.

## Change taxonomy

| Level | Change | Action |
| --- | --- | --- |
| 0 | Identical source | Reuse cache |
| 1 | Comments, whitespace, formatting | Reuse cache |
| 2 | Local variable or parameter rename | Reuse cache in release mode |
| 3 | Function body logic or control-flow change | Recompile the translation unit |
| 4 | Public signature, class layout, or exported header change | Recompile unit and dependents |

## Major modules

1. **AST analyzer** — Clang LibTooling visitor for functions, call sites, and includes.
2. **Normalizer** — canonical S-expression form with alpha-renamed local bindings.
3. **Fingerprint engine** — SHA-256 hashes and Merkle aggregation.
4. **Impact engine** — dependency DAG and affected-unit traversal.
5. **Cache/build engine** — JSON metadata, compiler invocation, object reuse, linking.

## Evaluation scenarios

Measure cache hit rate, AST overhead, build time, and recompilation reduction for
comments/formatting, local renames, function reordering, body edits, control-flow
edits, signature changes, and header layout changes.
