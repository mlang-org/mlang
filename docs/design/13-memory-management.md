# 13. Memory Management

This is the heart of MLang's value proposition: **memory safety with predictable,
pause-free latency, without asking the programmer to be a borrow-checker expert.**
The model is a hybrid of compile-time ownership and automatic reference counting,
with an optional backstop cycle collector.

## 13.1 The four guarantees

MLang statically and dynamically guarantees that safe code never exhibits:

1. **Memory leaks** (every value is dropped when its last owner/reference dies),
2. **Dangling pointers** (no reference outlives its referent),
3. **Double free** (a value is dropped exactly once),
4. **Use-after-free** (a freed value is never accessed).

In safe code these are *impossible*, not merely unlikely. `unsafe` code can break
them and is the only place that can, which is what makes auditing tractable
(Chapter 31).

## 13.2 The model, layer by layer

MLang applies the cheapest mechanism that is provably correct, in this order:

### Layer 0 - Stack and value types

Value types (`struct`, primitives, small tuples) live on the stack or inline in
their container. They are copied or moved by value and need no heap and no
counting. Most data in a well-written program never leaves this layer. Escape
analysis (Chapter 10) pushes even some `class` instances down to here.

### Layer 1 - Unique ownership and moves

A `class` instance with a single, statically tracked owner is a plain pointer to
a heap allocation with **no reference count at all**. Ownership transfers by move;
the compiler inserts exactly one `drop` at the end of the owner's scope. This is
RAII: deterministic, zero-overhead destruction. Passing such a value to a
function either moves it (transfers ownership) or borrows it.

### Layer 2 - Borrows

Instead of transferring ownership, code can borrow: `&T` (shared, read-only,
many) or `&mut T` (exclusive, mutable, one). Borrows are compile-time only
(they are plain pointers at runtime with no bookkeeping) and obey
aliasing-XOR-mutability, which is what makes data races and iterator invalidation
impossible (Chapter 15). Borrows never escape beyond the owner's lifetime; the
checker proves this.

### Layer 3 - Automatic reference counting

When a reference value is shared in a way unique ownership cannot track (stored in
two long-lived places, captured by an escaping closure, put in a collection of
shared things), the compiler represents it as `Rc<T>`: a heap allocation with a
strong count. The compiler inserts `retain` on acquisition and `release` on drop;
when the count hits zero the destructor runs and memory is freed - immediately and
deterministically, on the thread that released the last reference. There is no
collector pause.

Crucially, this choice is *inferred by default*. The programmer writes ordinary
code; the compiler decides per value whether it can stay at Layer 1/2 or must go
to Layer 3. The programmer can also be explicit (`Rc<T>`, `&T`) when they want
control.

### Layer 4 - Weak references and the optional cycle collector

Reference counting cannot reclaim cycles (A holds B holds A). MLang addresses this
two ways:

- **`Weak<T>`**: a non-owning reference that does not contribute to the strong
  count and reads as `T?` (null once the referent is gone). Idiomatic
  parent/child and observer graphs use `Weak` for the back-edges, so they never
  form a strong cycle. This is the recommended, zero-overhead solution and the
  one the standard library uses.
- **Optional cycle collector**: for code that cannot avoid strong cycles, an
  opt-in, concurrent **trial-deletion** cycle collector can be enabled
  (`--gc=cycles`). It runs only over the subset of objects that could be in a
  cycle (those whose count was decremented but stayed positive), not the whole
  heap, so it is bounded and incremental, not a stop-the-world tracing GC. Most
  programs never turn it on.

## 13.3 Why this beats "ownership only" and "ARC only"

- **Versus Rust (ownership only):** Rust is zero-overhead but pushes shared and
  cyclic graphs onto the programmer (`Rc`/`RefCell`/`Weak`, lifetimes in
  signatures). MLang keeps the zero-overhead path for the code that fits it but
  *automatically* falls back to ARC for the code that does not, so the learning
  cliff is gentler and signatures stay lifetime-free.
- **Versus Swift (ARC only):** Swift counts everything and relies on the
  optimizer to remove counts, which it often cannot across module boundaries.
  MLang's ownership layer means large amounts of code provably need *no* counting
  at all, and the counts that remain are aggressively elided (Chapter 10).
- **Versus a tracing GC:** no pauses, no heap over-provisioning, deterministic
  finalization (so `destructor` can close files/sockets reliably), and a cost
  model the programmer can read off the source. The rationale is
  [Chapter 14](14-why-no-gc.md).

## 13.4 Destruction order and RAII

When an owner's scope ends (normally, by `return`, by `break`, or by an exception
unwinding through it), the compiler runs destructors in reverse construction
order. `destructor()` is where files, sockets, locks, and FFI handles are
released. Because destruction is deterministic (not "whenever the GC gets around
to it"), RAII patterns - scoped locks, transactions, temp files - are reliable,
which a tracing GC fundamentally cannot promise.

## 13.5 The cost model, made explicit

| Operation | Cost |
|-----------|------|
| Create/destroy a value type | stack adjust; effectively free |
| Move a unique owner | pointer copy; one `drop` at scope end |
| Borrow (`&`/`&mut`) | pointer copy; nothing at runtime |
| Acquire/drop a shared `Rc<T>` | one (often elided) atomic inc/dec |
| `Weak<T>` upgrade | one atomic check + conditional inc |
| Cycle collection | only if `--gc=cycles`, bounded to cycle candidates |

The programmer can predict allocation and counting by reading the code, which is
the property that makes MLang suitable for latency-critical systems.

## 13.6 Safety proof sketch

Unique ownership is an affine type discipline (each owned value used at most once
as an owner), which by construction gives single-drop and no-use-after-move.
Borrows are checked against owner lifetimes (a borrow's region is contained in the
owner's), giving no-dangling. ARC maintains the invariant "strong count == number
of live strong references", giving free-exactly-when-unreferenced. Weak refs plus
the optional collector close the cycle gap. The combination yields the four
guarantees of 13.1; `unsafe` is the only escape and is the audited surface.
