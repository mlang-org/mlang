# 38. Benchmark Targets

This chapter quantifies "fast". It defines the workloads MLang is measured
against, the competitive bar, and the methodology, so that performance is a
tested, falsifiable property rather than a marketing adjective.

## 38.1 The competitive bar

MLang's target is **parity with Go, C#, Java, Kotlin, and Swift** across the
suite, and **within 5-15% of idiomatic C++** on arithmetic-bound code, while
keeping its safety guarantees. Concretely, per workload class:

| Workload class | Target vs C++ | Target vs Go/Java/C#/Kotlin/Swift |
|----------------|---------------|-----------------------------------|
| Arithmetic / numeric kernels | within 1.05-1.15x | meet or beat |
| Data structures (maps, vectors) | within 1.1-1.3x | meet or beat |
| Allocation-heavy / OOP | within 1.2-1.5x | meet or beat (no GC pauses) |
| Concurrency / async I/O | n/a (different model) | meet or beat throughput; far better tail latency |
| Startup time | within 1.1x | far better than JVM/CLR |
| Memory footprint (RSS) | within 1.2x | far better (no GC headroom) |

The latency claim is the differentiator: MLang targets **p99.9 latency with no
runtime-induced pauses**, which GC'd languages cannot guarantee (Chapter 14).

## 38.2 The benchmark suite

A mix of microbenchmarks (isolate one cost) and macrobenchmarks (realistic):

**Micro**
- `nbody`, `mandelbrot`, `spectral-norm` - floating-point and loops.
- `binary-trees` - allocation and deallocation churn (where no-GC determinism
  shows).
- `fannkuch`, `fasta` - integer and array work.
- `hashmap-insert/lookup`, `vector-push`, `sort` - standard collections.
- `string-concat`, `regex-match`, `json-parse` - text processing.

**Macro**
- `http-echo` and `http-json` - requests/sec and latency percentiles under load.
- `coroutine-churn` - millions of short tasks (scheduler + stackless-frame cost).
- `actor-pingpong` - channel throughput and scheduling latency.
- `compile-self` - the compiler compiling the standard library (build throughput).

## 38.3 Metrics captured

For every benchmark: wall-clock time (median + median-absolute-deviation),
allocations and bytes allocated, peak RSS, and - for the macro/latency ones - the
full latency distribution (p50/p90/p99/p99.9/max). Reporting the tail, not just
the mean, is deliberate: the tail is where the memory-model choice is visible.

## 38.4 Methodology

- **Same machine, pinned**: fixed CPU governor, isolated cores, ASLR controlled,
  hyperthreading documented, multiple runs with warmup and outlier rejection.
- **Idiomatic code per language**: each implementation is written the way a
  competent developer in that language would write it, not hand-tuned to win;
  tuned variants are reported separately and labeled.
- **Same compiler flags class**: release/`-O2`+LTO equivalents across languages.
- **Published harness**: the suite, inputs, and runner are in the repo so results
  are reproducible and disputable.

## 38.5 Regression gating

The micro suite runs in CI on a reference machine. A change that regresses a
benchmark beyond a per-benchmark threshold (default 3% for compute, tighter for
allocation counts which should be deterministic) fails the build. Performance is
thereby protected continuously, the way correctness is by tests. Macrobenchmarks
run nightly (they are noisier and slower) and trend-track rather than hard-gate.

## 38.6 Honesty clause

Where MLang loses, the suite says so and the docs explain why (Chapter 32.8):
bounds checks, debug overflow checks, and unavoidable ARC on shared graphs each
cost something, and the suite quantifies that cost rather than hiding it. A
benchmark MLang cannot yet win is tracked as a performance bug, not omitted. The
goal is a measured, trustworthy performance story, which is worth more than a
cherry-picked one.
