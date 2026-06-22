<div align="center">

# MLang

**A modern, statically-typed, memory-safe systems & application language.**

*Compiled. Null-safe. Ownership without a tracing GC. Coroutines built in.*

[![CI](https://github.com/mlang-org/mlang/actions/workflows/ci.yml/badge.svg)](https://github.com/mlang-org/mlang/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++23](https://img.shields.io/badge/compiler-C%2B%2B23-00599C.svg)](compiler/)
[![Spec](https://img.shields.io/badge/docs-design-success.svg)](docs/design/README.md)

</div>

---

## What is MLang?

MLang is a general-purpose programming language that aims for the *productivity*
of Kotlin/Swift and the *performance* of Go/C#, while guaranteeing memory safety
**without a stop-the-world garbage collector**. It compiles ahead-of-time to
native code through an LLVM backend.

```mlang
namespace com.example.hello

import std.io.Console

public fn main() {
    let name: "world"
    Console.println("Hello, ${name}!")
}
```

### Design pillars

- **Memory safety without a tracing GC.** A hybrid of *compile-time ownership*
  (move semantics + borrow checking) and *automatic reference counting* with
  compiler-elided counts. No leaks, no dangling pointers, no use-after-free, no
  double free — and predictable, pause-free latency. (An optional backup tracing
  collector can be switched on for cyclic graphs.)
- **Null safety by construction.** `T` is never null; `T?` is. Safe-navigation
  (`?.`), Elvis (`?:`), and the null-coalescing operators make absence explicit.
- **Structured concurrency.** First-class coroutines (`async`/`await`,
  `launch`), CSP-style `Channel<T>`, and a work-stealing scheduler.
- **One obvious way to build.** `mbuild` for builds, `mango` for packages,
  `mfmt` for formatting, `mls` for editor intelligence — all first-party.
- **Fast, parallel, incremental compilation.** The compiler is multi-threaded
  end to end with a persistent on-disk cache.

## Toolchain

| Tool             | Binary          | Purpose                              |
|------------------|-----------------|--------------------------------------|
| Compiler         | `mlangc`        | Lexing → … → native code generation. |
| Runtime          | `mlang-runtime` | Allocator, scheduler, intrinsics.    |
| Package manager  | `mango`         | Dependencies, publishing, registry.  |
| Build tool       | `mbuild`        | Project orchestration.               |
| Formatter        | `mfmt`          | Canonical code style.                |
| Language server  | `mls`           | LSP for any editor.                  |
| IDE extension    | —               | *MLang Language Support* (VS Code).  |

File extension: **`.m`**

## Quick start

```sh
# Build the toolchain (frontend + tools build without LLVM; add
# -DMLANG_ENABLE_LLVM=ON for native codegen)
cmake --preset dev
cmake --build --preset dev

# Tokenize / parse a program with the compiler driver
./build/dev/compiler/mlangc --emit=tokens examples/hello.m
./build/dev/compiler/mlangc --emit=ast    examples/hello.m

# Create and run a project (once mbuild/mango are wired to the backend)
mbuild new hello && cd hello
mbuild run
```

## Repository layout

```
mlang/
├── compiler/        # mlangc — the C++23 compiler
├── runtime/         # mlang-runtime — allocator, scheduler, intrinsics
├── stdlib/          # the standard library, written in MLang
├── tools/           # mfmt, mls, mbuild
├── packages/mango/  # package manager (git submodule -> mlang-org/mango)
├── editors/vscode/  # "MLang Language Support" extension
├── docs/            # design documents, language spec, EBNF grammar
├── examples/        # sample MLang programs
└── tests/           # cross-cutting language & end-to-end tests
```

## Documentation

The full language and compiler design lives in
**[`docs/design/`](docs/design/README.md)** — 39 chapters covering everything
from the type system and IR to the memory model, concurrency, ABI, and the
roadmap. The formal grammar is in
[`docs/grammar/mlang.ebnf`](docs/grammar/mlang.ebnf).

## Project status

MLang is in **early development**. The design is complete and the compiler
frontend (lexer, parser, AST, diagnostics) is functional; semantic analysis,
the IR, and the LLVM backend are under active construction. See the
[roadmap](docs/design/33-roadmap.md) and
[development phases](docs/design/37-development-phases.md).

## License

MLang is distributed under the [MIT License](LICENSE).
