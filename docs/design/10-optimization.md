# 10. Optimization Passes

Optimization happens in two tiers: **MLang-aware passes on MIR** (this chapter)
and **target passes in LLVM** (Chapter 11). The division of labor is the whole
point: MIR passes use language facts LLVM cannot see; LLVM does instruction
selection, register allocation, and target tuning.

## 10.1 The MIR pass pipeline

Passes run in a fixed order with a fixed-point loop around the cheap local
passes. `-O0` runs almost none (fast builds, faithful debugging); `-O2` is the
default for release; `-O3` adds the aggressive, compile-time-expensive passes.

```
mem2reg / SSA cleanup
  → constant folding + propagation
  → algebraic simplification
  → dead code elimination
  → CFG simplification (block merging, jump threading)
  → inlining (with a cost model)
  → [fixed point of the above]
  → escape analysis              → stack promotion of non-escaping allocations
  → ARC optimization             → retain/release elision and motion
  → devirtualization             → speculative + provable
  → bounds-check elimination
  → loop optimization            → LICM, unrolling, unswitching, IV simplification
  → scalar replacement of aggregates (SROA)
  → tail-call optimization
  → final DCE + verify
```

## 10.2 The high-value, language-specific passes

### Constant folding and propagation

Folds operations on known constants and propagates them. Backed by the same
const-evaluator as `const fn` (Chapter 8), so it folds user `const fn` calls,
not just primitives. Overflow during folding respects the checked-arithmetic mode
so it never silently changes semantics.

### Inlining

A bottom-up inliner with a cost/benefit model: it estimates callee size after
simplification and the call-site benefit (constant arguments, devirtualization
opportunities, single-use closures). Small functions, single-call-site
functions, and accessors are inlined aggressively because MLang code is written
with many tiny functions and properties; not inlining them would leave easy
performance on the floor. Cross-module inlining pulls callee MIR from `.mmi`.

### Escape analysis

Determines whether an allocation's lifetime is bounded by the current function.
Non-escaping heap allocations are **promoted to the stack** (or scalarized away
entirely), which removes allocator traffic and ARC operations. This is the
single most important pass for making idiomatic, allocation-heavy code fast, and
it directly supports the no-GC story: many "objects" never reach the heap at all.

### ARC optimization

Because `retain`/`release`/`drop` are explicit in MIR (Chapter 9), the optimizer
treats them as ordinary instructions and applies:

- **Redundant pair elimination**: a `retain` immediately followed (on all paths)
  by a `release` of the same value cancels.
- **Motion out of loops**: hoist a `retain`/`release` pair of a loop-invariant
  value out of the loop (retain once, release once).
- **Uniqueness propagation**: when a value is provably uniquely owned, its
  refcount traffic is removed and its COW buffers mutate in place.

In well-typed idiomatic code, ARC optimization removes the large majority of
naively-inserted counts, which is why MLang's ARC is competitive with manual
memory management rather than a constant tax (Chapter 13).

### Devirtualization

Interface/virtual calls become direct calls when the receiver's concrete type is
known (from construction, `final`, sealed hierarchies, or inlining). A direct
call is then itself an inlining candidate, so devirtualization and inlining feed
each other. Where the type is only *probably* known, a guarded speculative
devirtualization is emitted (a type check fast path + a virtual slow path).

### Bounds-check elimination

Array indexing is bounds-checked for safety (Chapter 31). The optimizer removes
checks it can prove redundant: loops with an induction variable bounded by the
array length, repeated indexing with the same proven-in-range index, and indices
derived from a prior checked access. This keeps safe array code at C-like speed
in the common loop shapes.

### Loop optimizations

Loop-invariant code motion, unrolling (full for small trip counts, partial
otherwise), unswitching (hoist a loop-invariant branch out of the loop), and
induction-variable simplification. Many of these also exist in LLVM, but running
them on MIR first lets later MIR passes (bounds-check elimination, ARC motion)
benefit from the cleaned-up loop.

## 10.3 Whole-program and link-time optimization

With `-flto`, optimization continues across module boundaries at link time:
cross-module inlining, whole-program devirtualization (the final set of types
implementing an interface is known), and dead-code elimination of unreachable
public symbols. LTO is optional because it costs build time; `-O2` without LTO is
the default.

## 10.4 Pass infrastructure

Passes implement a small `MirPass` interface and declare what they preserve
(e.g. "preserves CFG", "preserves SSA"), so a pass manager can skip recomputing
analyses that are still valid. Analyses (dominator tree, alias info, loop info,
liveness) are computed lazily and cached, and invalidated only by passes that
say they change the relevant structure. This is the same architecture as LLVM's
new pass manager and `rustc`'s analysis caching, chosen because it is proven.

## 10.5 Correctness

Every pass is bracketed by the MIR verifier (Chapter 9) in debug/CI builds, and
the optimizer has a differential-testing harness: randomly generated programs are
run interpreted on unoptimized MIR and compiled at each `-O` level; a divergence
is a miscompilation bug. Optimization that changes observable behavior is the
worst kind of compiler bug, so it gets the heaviest testing.
