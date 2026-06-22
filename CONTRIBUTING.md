# Contributing to MLang

Thank you for your interest in improving MLang! This document explains how the
project is organized and how to get your changes merged.

## Repository layout

| Path          | Contents                                                        |
|---------------|----------------------------------------------------------------|
| `compiler/`   | `mlangc` — the compiler, written in C++23.                      |
| `runtime/`    | `mlang-runtime` — the language runtime (allocator, scheduler).  |
| `stdlib/`     | The standard library, authored in MLang (`*.m`).               |
| `tools/`      | `mfmt`, `mls`, `mbuild`.                                        |
| `packages/mango/` | Git submodule for the `mango` package manager.             |
| `docs/`       | Design documents, language spec, and the EBNF grammar.         |
| `examples/`   | Sample MLang programs.                                          |
| `tests/`      | Cross-cutting language and end-to-end tests.                    |

## Toolchain prerequisites

- A C++23 compiler (GCC ≥ 14, Clang ≥ 17, or MSVC ≥ 19.38).
- CMake ≥ 3.28.
- (Optional) LLVM ≥ 17 development packages to build the native backend
  (`-DMLANG_ENABLE_LLVM=ON`).

## Building

```sh
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

Without LLVM installed, the compiler frontend (lexer, parser, semantic
analysis) and all tools still build and run; only native code generation is
disabled.

## Commit conventions

We use [Conventional Commits](https://www.conventionalcommits.org/). The commit
type is one of `feat`, `fix`, `docs`, `refactor`, `perf`, `test`, `build`,
`ci`, or `chore`, optionally scoped, e.g.:

```
feat(parser): support trailing commas in argument lists
fix(lexer): handle CRLF inside raw string literals
docs(design): clarify ownership transfer rules
```

## Coding standards

- C++ code is formatted with `clang-format` using the repository `.clang-format`.
- MLang code is formatted with `mfmt`.
- Public APIs require doc comments (`///`).
- Every bug fix should come with a regression test.

## Code review

All changes land via pull request and require at least one approving review and
a green CI run before merge.
