# 1. Language Philosophy

MLang exists to answer a single question: *can a language feel as productive as
Kotlin or Swift while running as predictably as Go or C, without asking the
programmer to choose between safety and control?* Every decision in this
document is downstream of that question.

## 1.1 Goals

1. **Memory safety with predictable latency.** Programs must be free of
   dangling pointers, use-after-free, double-free, and data races *by
   construction*, and they must achieve this without a stop-the-world tracing
   collector on the hot path. Latency-sensitive code (games, trading, audio,
   network proxies) should never suffer an unbounded pause that the programmer
   did not ask for.

2. **Readability first.** Code is read far more often than it is written. The
   surface syntax is small, regular, and free of ceremony. There is exactly one
   idiomatic way to format a program (`mfmt`), one way to build it (`mbuild`),
   and one way to fetch dependencies (`mango`).

3. **Statically typed, with inference where it helps.** Types are checked at
   compile time and erased only where it is provably safe. Local inference
   removes redundancy; signatures remain explicit so that APIs are
   self-documenting.

4. **Null safety is not optional.** The type `T` can never hold the absence of a
   value. Absence is expressed only through `T?`, and the compiler forces every
   `T?` to be handled before it is used as a `T`.

5. **Concurrency that scales and composes.** Coroutines, channels, and
   structured concurrency are first-class. Cancellation propagates along the
   call tree. The cost of a suspended task is bytes, not a kernel thread.

6. **Native performance.** Ahead-of-time compilation through LLVM, aggressive
   monomorphization, escape analysis, and a runtime with no managed heap walk.
   The performance target is parity with Go/C#/Java/Kotlin/Swift, and within
   reach of C++ for arithmetic-bound code (see [Chapter 38](38-benchmarks.md)).

7. **A toolchain, not just a compiler.** A language is only as good as the day
   it takes to learn its build, test, format, and editor story. All of these
   ship in the box and are version-locked to the compiler.

## 1.2 Non-goals

- **MLang is not a research vehicle for a novel type theory.** It uses
  well-understood machinery (System F with bounded quantification, flow typing,
  affine ownership) chosen for predictability, not novelty.
- **MLang does not aim to be the fastest language in every microbenchmark.** It
  trades the last few percent of raw throughput for safety and ergonomics where
  the two conflict, and documents exactly where that line is.
- **MLang is not a scripting language.** There is no eval, no implicit global
  mutable state, and no dynamic typing escape hatch beyond the explicit
  `reflect` and `dynamic` facilities.
- **MLang does not promise C++ ABI compatibility.** It interoperates with C
  (and a curated C++ subset) through an explicit FFI boundary
  ([Chapter 30](30-ffi.md)), not by pretending to be C++.

## 1.3 Guiding principles

- **Safe by default, unsafe by request.** Unsafe operations exist (raw
  pointers, manual lifetime control, FFI) but live behind the `unsafe` keyword
  and are auditable with a single grep.
- **Make the right thing the easy thing.** Immutability (`let`) is shorter to
  type than mutability (`var`). Value types are the default for small data.
  Errors are values, propagated with a single `?` operator.
- **No hidden costs.** Every allocation, every reference-count operation, every
  copy is either visible in the source or eliminated by the optimizer. The cost
  model is teachable.
- **Errors are part of the language, not an afterthought.** Diagnostics carry a
  stable code, a span, a primary message, secondary notes, and where possible a
  machine-applicable fix (see [Chapter 4](04-lexer.md) and
  [Chapter 8](08-semantic-analysis.md)).
- **One language, many scales.** The same language writes a 10-line script and a
  million-line service. Modules, visibility, and packages exist so the second is
  not the first with more pain.

## 1.4 Influences and what was taken from each

| Language | What MLang borrows | What it deliberately rejects |
|----------|--------------------|------------------------------|
| Rust     | Ownership, move semantics, exhaustive `match`, errors as values | Lifetime annotations in signatures; steep borrow-checker learning curve |
| Kotlin   | Null safety, `when`/`match` ergonomics, extension functions, coroutines | JVM dependency; nullable platform types |
| Swift    | ARC as a baseline, value semantics, protocols | Whole-module optimization fragility; ObjC bridging |
| Go       | Fast compiles, channels, goroutine-style cheap tasks, one formatter | Lack of generics history; `nil` interfaces; minimal type system |
| C#       | `async`/`await` surface, properties, LINQ-style collection API | Heavy runtime; tracing GC on the hot path |
| C++      | Zero-overhead abstractions, value types, RAII (as destructors) | Header model; undefined behavior surface; manual memory |

## 1.5 The shape of an MLang program

The following is idiomatic MLang. It is null-safe, uses structured
concurrency, propagates errors with `?`, and frees all resources
deterministically without any explicit `free`.

```mlang
namespace com.example.crawler

import std.io.Console
import std.http.HttpClient
import std.collections.List
import std.concurrent.{ async, await, gather }

async fn fetchTitle(client: HttpClient, url: String): String? {
    let response: await client.get(url)?
    return response.body.between("<title>", "</title>")
}

public async fn main() {
    let client: HttpClient.shared()
    let urls: ["https://a.example", "https://b.example", "https://c.example"]

    let titles: await gather(urls.map((u) => fetchTitle(client, u)))

    for (title in titles) {
        match (title) {
            case let t: String => Console.println("ok: ${t}")
            case null         => Console.println("no title")
        }
    }
}
```

Everything in this snippet is explained in the chapters that follow. The point
here is the *feel*: declarative, safe, and free of memory or lifetime
bookkeeping, yet it compiles to native code with no tracing GC behind it.
