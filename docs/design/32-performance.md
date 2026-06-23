# 32. Performance Strategies

This chapter collects, in one place, the design choices that make MLang fast. The
target (parity with Go/C#/Java/Kotlin/Swift, within reach of C++ for arithmetic
code) is quantified in [Chapter 38](38-benchmarks.md); here is *how* it is met.

## 32.1 The performance philosophy

MLang aims for a **transparent cost model** and **no abstraction tax**. The
programmer should be able to predict performance from the source, and idiomatic,
high-level code should compile to roughly what a careful systems programmer would
write by hand. Every strategy below serves one of those two goals.

## 32.2 Compile-time strategies

- **Monomorphization** (Chapter 18): generics compile to concrete code, no boxing,
  so a `List<Int>` is as fast as a hand-written int array.
- **Aggressive inlining** (Chapter 10): MLang code is written with many tiny
  functions and properties; inlining them removes call overhead and unlocks
  downstream optimization.
- **Escape analysis -> stack allocation** (Chapter 10): non-escaping objects never
  reach the heap, cutting allocator traffic and ARC operations.
- **ARC elision** (Chapter 10, 13): the large majority of naively-inserted
  reference counts are proven redundant and removed.
- **Devirtualization** (Chapter 10): interface/virtual calls become direct (and
  then inlinable) when the type is known, which it often is thanks to the
  default-`internal` closed-world (Chapter 27).
- **Bounds-check elimination** (Chapter 10): safe array loops run check-free where
  the index is provably in range.
- **Language-aware MIR before LLVM** (Chapter 9): high-value optimizations run
  while nullability/ownership/exhaustiveness facts still exist, then LLVM does
  target tuning.

## 32.3 Runtime strategies

- **No tracing GC** (Chapter 14): no collector pauses, no barriers on heap access,
  no 2-5x memory headroom.
- **Deterministic, immediate reclamation** (Chapter 13): memory returns to the
  allocator the instant the last reference dies, keeping the working set small and
  cache-warm.
- **A fast, thread-caching allocator** (Chapter 12): lock-free fast path for small
  objects.
- **Stackless coroutines** (Chapter 16): a suspended task costs bytes, so
  high-concurrency servers scale without thread or stack overhead.
- **Value semantics + COW** (Chapter 20): collections and strings avoid copies
  until mutated, and mutate in place when uniquely owned.

## 32.4 Data-layout strategies

- **Struct field reordering** to minimize padding, and **niche-optimized
  enums/options** (Chapter 29), so data structures are compact and cache-friendly.
- **Inline storage** for small collections and small-string optimization for
  short `String`s, avoiding heap allocation for the common small case.
- **Cache-aware standard collections** (open-addressing hash maps, contiguous
  vectors) chosen for real-hardware behavior, not asymptotics alone.

## 32.5 Concurrency and I/O strategies

- **M:N work-stealing scheduler** (Chapter 12) keeps all cores busy with cheap
  tasks and good cache locality (LIFO local queues).
- **Async I/O via the reactor** so a blocked request costs a parked coroutine, not
  a thread.
- **Structured concurrency** (Chapter 15) makes parallelism easy to add correctly,
  so programs actually use the cores they have.

## 32.6 Build-time performance (developer throughput)

Performance is not only runtime; the edit-build-test loop matters:

- **Parallel + incremental + cached** compilation (Chapter 3): rebuilds are
  proportional to the change.
- **`mbuild check`** (frontend only) for sub-second type feedback.
- **`.mmi` separation** so body edits do not cascade rebuilds (Chapter 2).

## 32.7 Measuring and protecting performance

- A benchmark suite (Chapter 38) runs in CI; a regression beyond a threshold fails
  the build, so performance is a tested property, not a hope.
- `@Bench` (Chapter 26) gives users the same rigorous measurement for their code.
- Profile-guided optimization (PGO) and LTO (Chapter 11) are available for the
  last increments on hot binaries.

## 32.8 Where MLang deliberately is not the fastest

MLang keeps bounds checks, overflow checks (in debug), and ARC where ownership
cannot be proven; these have a small, *bounded, documented* cost that buys safety.
The language gives explicit tools to recover the last few percent where it
genuinely matters (`unsafe` indexing, wrapping arithmetic, `&`-borrows to drop
ARC, `@erased` off), so the safe default is fast and the unsafe ceiling is C-class
- a dial, not a wall.
