# 20. Standard Library Design

The standard library is, by design, **mostly written in MLang itself**. The
compiler provides a small set of intrinsics and the runtime provides the floor
(Chapter 12); everything else - collections, strings, I/O, JSON, HTTP, time,
math, concurrency utilities - is `.m` code in `stdlib/`. This is both a
dogfooding strategy (the language must be good enough to write its own library)
and a portability strategy (the library is target-independent).

## 20.1 Module map

```
std
├── core         intrinsic-backed primitives, Option/Result, Comparable, Hashable
├── collections  List, MutableList, Set, MutableSet, Map, HashMap, TreeMap,
│                Queue, Deque, Stack, LinkedList, Vector, HashSet
├── string       String, StringView, StringBuilder, Char, formatting
├── io           Console, Reader, Writer, BufferedReader, streams
├── fs           File, Path, Directory, metadata, permissions
├── math         numeric functions, BigInt, BigDecimal, constants, random seeds
├── time         Instant, Duration, DateTime, ZonedDateTime, Clock, Stopwatch
├── concurrent   Task, Future, Channel, Mutex, RwLock, Atomic, Semaphore, gather, select
├── json         parse/serialize, @Serializable, JsonValue, streaming
├── http         HttpClient, HttpServer, Request, Response, headers, routing
├── reflect      typeOf, TypeInfo, FieldInfo, MethodInfo, Dynamic
├── crypto       hashes (SHA-2/3, BLAKE3), HMAC, AEAD, random (CSPRNG), KDF
├── regex        Regex, Match, compiled patterns
├── log          Logger, levels, structured fields, sinks
├── config       layered configuration (env, file, defaults)
├── serial       serialization framework (binary, JSON-backed)
└── test         the test framework (Chapter 26)
```

## 20.2 Design principles

- **Immutable by default, mutable on request.** `List` is immutable; `MutableList`
  is the mutable sibling. Read-only views are cheap. This mirrors the language's
  `let`/`var` bias and makes sharing across tasks safe.
- **Value semantics with copy-on-write.** `String`, `List`, and `Map` are
  value-semantic but back themselves with shared, copy-on-write buffers, so
  passing them is cheap and mutation of a uniquely-owned buffer is in-place
  (uniqueness comes from the ARC `unique` flag, Chapter 13).
- **One collection vocabulary.** A single set of higher-order operations
  (`map`, `filter`, `fold`, `flatMap`, `any`, `all`, `take`, `drop`, `zip`,
  `groupBy`, `sorted`) works across all sequences via the `Sequence`/`Iterable`
  interfaces and extension methods (Chapter 18). They are lazy where it pays
  (`Sequence`) and eager where it is clearer (`List`).
- **Errors as values.** Fallible APIs return `Result<T, E>`; nothing in the
  standard library throws a panic for a recoverable condition (Chapter 17).
- **Async-first I/O.** `io`, `fs`, `http` expose `async` APIs that integrate with
  the scheduler; synchronous wrappers exist for scripts.
- **No hidden allocation.** APIs document their allocation behavior; builders and
  views let hot code avoid temporaries.

## 20.3 The collection example

```mlang
import std.collections.{ List, Map }

let scores: Map<String, Int> = ["alice": 90, "bob": 75, "carol": 88]

let passing: List<String> =
    scores
        .filter((name, score) => score >= 80)
        .map((name, _) => name)
        .sorted()

// passing == ["alice", "carol"]
```

This reads like Kotlin/Swift, compiles to monomorphized code with the closures
inlined (Chapter 10), and allocates only the final list (the intermediate
pipeline is fused where the optimizer can prove it safe).

## 20.4 The intrinsic boundary

The compiler exposes a minimal intrinsic surface that the library builds on:
`@intrinsic` functions for raw memory (`unsafe`), atomic operations, checked
arithmetic, `sizeOf`/`alignOf`, the ARC operations, coroutine primitives, and the
runtime calls of Chapter 12. Library code uses these inside safe abstractions
(e.g. `Vector<T>` wraps raw allocation; `Mutex<T>` wraps atomics) so that user
code never touches `unsafe` to use the standard collections.

## 20.5 Stability and versioning

The standard library follows the language's semantic versioning. Additions are
minor; removals/signature changes are major and gated by an RFC and a deprecation
cycle (`@deprecated(since:, use:)`). The `core` module is the most conservative;
higher modules (`http`, `crypto`) may evolve faster behind a stability attribute.
The library's public API is snapshotted and diffed in CI so an accidental
breaking change cannot merge.

## 20.6 Why write it in MLang

A standard library in the language itself is the strongest possible test of the
language's expressiveness and performance, keeps the library portable to every
target the backend supports without rewriting, and lets the optimizer inline
across the user/library boundary (the library ships as `.mmi` with inline bodies,
Chapter 2). The alternative (a large C/C++ core) would fragment the cost model
and the toolchain. The only things kept native are those that genuinely cannot be
expressed safely in MLang, which is the runtime floor and a handful of intrinsics.
