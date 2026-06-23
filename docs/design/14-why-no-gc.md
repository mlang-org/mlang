# 14. Why Not a Tracing Garbage Collector

This chapter justifies the single most consequential runtime decision in MLang:
the default execution model has **no tracing garbage collector**. This is not
ideology; it is a response to the specific costs a tracing GC imposes and the
specific guarantees MLang wants to make.

## 14.1 What a tracing GC costs

A tracing GC (mark-sweep, mark-compact, generational, or concurrent) periodically
walks the live object graph to find garbage. Even the best modern collectors
(ZGC, Shenandoah, Go's collector) pay, in some combination:

1. **Latency tail.** Even "sub-millisecond pause" collectors add latency: read/
   write barriers on every heap access, safepoint polling, and occasional longer
   pauses under load. For a 99.9th-percentile-sensitive system (trading, ads,
   games, audio, real-time control), the tail is the product.
2. **Throughput overhead from barriers.** Concurrent collectors keep the mutator
   running by inserting load/store barriers. These are a tax on *every* heap
   access, paid whether or not collection is happening.
3. **Memory headroom.** Tracing collectors need slack (often 2-5x the live set)
   to amortize collection cost. That memory is unavailable to the application and
   raises cloud cost per instance.
4. **Non-deterministic finalization.** Finalizers/destructors run "eventually",
   so a GC'd language cannot use RAII to manage non-memory resources (file
   handles, sockets, locks) reliably; you end up with `defer`/`using`/`try-with-
   resources` bolted on, which is exactly the determinism MLang wants by default.
5. **Hidden cost model.** A programmer cannot look at a function and know its
   allocation/pause behavior; performance work becomes GC-tuning archaeology.

## 14.2 What MLang wants instead

- **Bounded, predictable latency.** No pause the programmer did not write. A
  destructor runs at a point you can see in the source.
- **A readable cost model.** Allocation and reference counting are visible or
  provably elided (Chapter 13).
- **Deterministic resource management.** RAII for files/sockets/locks, for free,
  because destruction is deterministic.
- **Low memory footprint.** No 2-5x headroom; memory is released when the last
  reference dies.

Ownership + ARC (Chapter 13) delivers all four. That is why it is the default.

## 14.3 But isn't reference counting "also a GC", and slower?

Reference counting *is* a form of automatic memory management, yes. The common
critiques and MLang's answers:

- *"RC is slower than a good tracing GC on throughput."* True for naive,
  always-counting RC. MLang's counts are (a) avoided entirely for value types and
  uniquely-owned values (Layers 0-2), and (b) aggressively elided by the
  optimizer for the rest (Chapter 10). The residual counting is on genuinely
  shared objects only, which are a minority in typical code.
- *"RC can't collect cycles."* Correct, and addressed by `Weak<T>` (the idiomatic
  fix, zero overhead) plus an opt-in bounded cycle collector (Chapter 13). The
  language is designed so cycles are rare and the back-edge convention is natural.
- *"RC has cache-unfriendly count churn."* Counts are co-located with the object
  header and the bulk are removed by elision; what remains is far less memory
  traffic than a collector scanning the live set.

## 14.4 The honest trade-offs

A tracing GC is genuinely better at one thing: **mutating large, highly-shared,
cyclic object graphs with no programmer thought about ownership.** Workloads that
look like that (some compilers, some graph databases, some simulations) are where
a tracing GC shines and where MLang's model asks for either `Weak` edges or the
opt-in collector. MLang does not pretend this case away; it makes the collector
*available* (`--gc=cycles`) for exactly these programs, while keeping it *off* by
default for everyone else.

It is also true that ARC requires the compiler to do more work (ownership
analysis, ARC elision) than emitting allocations for a GC to clean up later. That
compile-time cost buys runtime predictability, which is the trade MLang chooses.

## 14.5 The escape hatch, not the default

MLang therefore ships *with* an optional collector but *not as* the execution
model:

- Default: ownership + ARC, no tracing, no pauses.
- `--gc=cycles`: add the bounded concurrent cycle collector for cyclic graphs.
- Future research target (Chapter 33): a region/arena allocator API for
  bulk-lifetime workloads, as a third point on the curve.

The principle is consistent with the language philosophy (Chapter 1): the safe,
predictable thing is the default; the powerful, situational thing is one explicit
flag away. We refuse to impose a whole-program pause on the 95% of programs that
do not need it to serve the 5% that do, when the 5% can opt in.
