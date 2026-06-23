# 18. Generics

Generics give MLang parametric polymorphism without boxing or runtime type tests
on the hot path. This chapter covers the implementation strategy
(monomorphization with selective erasure), constraints, variance, and the related
extension mechanism.

## 18.1 Surface syntax

```mlang
class List<T> { ... }
class Cache<K: Hashable & Equatable, V> { ... }

fn map<T, R>(xs: [T], f: (T) -> R): [R] { ... }

fn clamp<T>(x: T, lo: T, hi: T): T where T: Comparable {
    return x < lo ? lo : (x > hi ? hi : x)
}
```

- Type parameters are listed in `<...>` on types and functions.
- Constraints use `:` with `&` for multiple bounds, or a `where` clause for room.
- Bounds are interfaces (most common) and, rarely, a class (to require a base).

## 18.2 Monomorphization by default

Each *used* instantiation is compiled to specialized code: `List<Int>` and
`List<String>` produce distinct, fully concrete machine code. Consequences:

- **No boxing.** A `List<Int>` stores `i64`s inline, not pointers to boxed
  integers. This is the difference between Java-style generics (erased, boxed) and
  MLang's, and it is decisive for numeric and data-structure performance.
- **The optimizer sees concrete types**, so inlining, devirtualization, and
  bounds-check elimination all fire inside generic code as if it were
  hand-written for that type.
- **Cost: code size and compile time.** Many instantiations multiply code. This
  is mitigated by (a) identical-instantiation merging (`List<Int>` compiled once
  program-wide via the type interner and the cache), (b) the linker folding
  identical machine code (`ICF`), and (c) selective erasure (below).

## 18.3 Selective erasure (dictionary passing)

For instantiations where code size matters more than the last bit of speed, a
generic can be compiled **once** in an erased form that passes a *witness table*
(a dictionary of the constraint's methods) at runtime, the way the value is
accessed through pointers. The compiler chooses, or the programmer annotates
(`@erased`), per type parameter. This is the Swift approach (specialize hot,
erase cold), and it gives a tunable point on the speed/size curve rather than
forcing the Rust (always specialize) or Java (always erase) extreme.

## 18.4 Constraints and conformance

A constraint `T: Hashable` means `T` must implement the `Hashable` interface. At
the call site the compiler proves conformance and, for monomorphized code, inlines
the concrete methods; for erased code, it passes the witness table. Conformance
can be:

- **Declared**: `class Point impl Hashable { ... }`.
- **Retroactive via extension**: implement an interface for a type you do not own
  (Chapter 18.6), as long as either the type or the interface is local (the
  coherence/orphan rule), which prevents two libraries from giving conflicting
  conformances.

## 18.5 Variance (recap and enforcement)

Declared as `out T` (covariant), `in T` (contravariant), or bare (invariant);
see [Chapter 7](07-type-system.md). The compiler checks that `out` parameters
appear only in output positions and `in` only in input positions across the whole
type, so a declared variance can never be unsound. Use-site variance
(`List<out Animal>`) covers types that cannot pick one variance.

## 18.6 Extensions

Extensions add methods (and interface conformances) to an existing type:

```mlang
extension String {
    fn isNumeric(): Bool {
        return this.length > 0 && this.chars().all((c) => c.isDigit())
    }
}

extension<T: Comparable> List<T> {
    fn sorted(): List<T> { ... }   // generic extension
}
```

Extension methods are resolved statically (no runtime dispatch unless the method
is also declared in an interface the value is used through). They are the
mechanism behind much of the collection API (Chapter 20) and behave like Kotlin/
Swift extensions: scoped by import, non-virtual, zero-overhead.

## 18.7 Generic specialization across modules

A generic's body lives in its module's `.mmi` so downstream modules can
instantiate it. When module B uses `List<B.Widget>`, the specialization is
generated in B's compilation (the body comes from A's `.mmi`), and the cache keys
it by `(generic, type args)` so it is built once even if many modules use the same
instantiation. This keeps separate compilation and monomorphization compatible.

## 18.8 Const generics (planned)

Array sizes and other compile-time values can be generic parameters:
`struct Matrix<T, const R: Int, const C: Int>`. Const generics use the const
evaluator (Chapter 8) and are on the roadmap (Chapter 33) after the value-generic
system is stable.
