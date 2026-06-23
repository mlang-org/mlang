# 7. Type System

MLang is statically and (mostly) nominally typed, with structural elements for
function types and tuples. The type system is built on System F with bounded
quantification (generics with constraints), extended with non-null types,
flow-sensitive narrowing, and definite-variance generics.

## 7.1 The type lattice

```
                         Any
              ┌───────────┼────────────┐
           AnyRef       AnyVal      (Nothing is the bottom)
          (classes,   (structs,
          interfaces)  primitives)
```

- `Any` is the top type. `Nothing` is the uninhabited bottom type (the type of
  `throw`, infinite loops, and `return`); it is a subtype of every type, which is
  what lets `let x: T = cond ? value : throw E()` type-check.
- `AnyRef` is the supertype of all reference (heap, identity) types; `AnyVal` of
  all value types. This split drives copy-vs-move and ARC decisions
  (Chapter 13).

## 7.2 Primitive and built-in types

| Category | Types |
|----------|-------|
| Signed integers | `i8 i16 i32 i64` (and `int` = `i64`) |
| Unsigned integers | `u8 u16 u32 u64` (and `uint` = `u64`) |
| Floating point | `f32 f64` (and `float` = `f64`) |
| Other scalars | `bool`, `char` (a Unicode scalar value), `byte` = `u8` |
| Text | `String` (UTF-8, value-semantic, COW), `StringView` |
| Aggregates | tuples `(A, B)`, arrays `[T]`, fixed arrays `[T; N]`, maps `[K: V]` |
| Functions | `(A, B) -> R`, `async (A) -> R` |
| Unit/never | `void` (the unit type, one value) and `Nothing` |

Integer arithmetic is **checked by default** in debug builds (overflow panics)
and wraps only via explicit `wrapping_*`/`&+` operators or in release builds when
the programmer opts in. This is a safety/performance dial documented in
[Chapter 31](31-security.md).

## 7.3 Null safety

Nullability is a first-class type constructor, not a runtime sentinel:

- `T` is the non-null type. A value of type `T` is statically guaranteed to be
  present.
- `T?` is the nullable type, represented as an option. For reference types `T?`
  is a possibly-null pointer (zero overhead); for value types it is a niche-
  optimized optional (no extra word when `T` has spare bit patterns).

The operators:

```mlang
let a: String? = maybe()
let len: Int = a?.length ?? 0       // safe-nav + null-coalescing
let b: String = a ?: "default"      // Elvis
let c: String = a!                  // non-null assertion (panics if null)
if (a != null) { use(a) }           // smart-cast: a is String inside the block
```

`a!` is the only way to go from `T?` to `T` without a check, and it is a checked
operation that panics with a precise location if violated. There is no implicit
nullable-to-non-null coercion anywhere in the language.

## 7.4 Flow-sensitive typing (smart casts)

The type checker tracks a *flow type* for each variable along each control path.
After `if (x != null)`, `x is Dog`, or a `match` arm, the variable's type is
narrowed in the branch where the condition holds:

```mlang
fn area(shape: Shape): Float {
    return match (shape) {
        case let c: Circle    => 3.14159 * c.radius * c.radius
        case let r: Rectangle => r.width * r.height
    }   // exhaustive: Shape is a sealed hierarchy, so no default needed
}
```

Narrowing is invalidated by any write to the variable or any call that could
mutate it; the analysis is conservative and sound. Only `let` (immutable)
bindings get the strongest narrowing.

## 7.5 Type inference

Inference is **local and bidirectional**, not global Hindley-Milner. The rules:

- Local `let`/`var` without an annotation infer from the initializer:
  `let n: 10` is `Int`, `let xs: [1, 2, 3]` is `[Int]`.
- Function *parameter* and *return* types are never inferred across function
  boundaries; signatures are explicit. This keeps inference fast, errors local,
  and APIs readable.
- Lambda parameter types are inferred from the expected type (the context),
  e.g. inside `list.map((x) => x * 2)`, `x` is inferred from the element type.
- Generic type arguments are inferred at call sites by unifying argument types
  with the declared parameter types.

The deliberate choice to require signatures is an ergonomics-vs-locality
trade-off: global inference gives slightly terser code but produces errors far
from their cause and slows the checker. MLang optimizes for big codebases.

## 7.6 Generics

Generics are parametric polymorphism with **bounded quantification**:

```mlang
class Cache<K: Hashable & Equatable, V> { ... }

fn max<T: Comparable>(a: T, b: T): T {
    return a > b ? a : b
}
```

- Constraints (`T: Bound`) are interface (and class) bounds; multiple bounds are
  combined with `&`. `where` clauses express constraints that need more room.
- Generics are **monomorphized** by default (Chapter 18): each used instantiation
  gets specialized code, so there is no boxing and the optimizer sees concrete
  types. Code-size-sensitive instantiations can opt into a dictionary-passing
  (type-erased) representation.

## 7.7 Variance

Type parameters declare variance, checked at definition and use:

```mlang
interface Producer<out T> { fn next(): T }        // covariant
interface Consumer<in T>  { fn accept(value: T) } // contravariant
class Box<T> { ... }                               // invariant (default)
```

- `out T` (covariant): `Producer<Dog>` is a subtype of `Producer<Animal>`. `T`
  may appear only in output position.
- `in T` (contravariant): `Consumer<Animal>` is a subtype of `Consumer<Dog>`.
  `T` may appear only in input position.
- Bare `T` is invariant. Use-site variance (`Box<out Animal>`) is also available
  for the cases where a class cannot commit to a single variance.

The compiler verifies the in/out position rules so an unsound variance
declaration is a compile error, not a runtime cast failure.

## 7.8 Subtyping

Subtyping arises from: class inheritance (`ext`), interface implementation
(`impl`), `Nothing <: T <: Any`, nullable widening (`T <: T?`), variance, and
function subtyping (parameters contravariant, results covariant). Subtyping is
*nominal* for classes/interfaces and *structural* for functions and tuples.
There is no implicit numeric subtyping: `i32` is not a subtype of `i64`;
widening is an explicit `as` or a literal that fits.

## 7.9 Type representation in the compiler

Types are **interned**: a canonical `Type*` is created once per distinct type and
reused, so type equality is pointer equality and the checker's hash maps key on
pointers. The interner is concurrent (Chapter 3). Generic instantiations are
interned with their arguments, so `List<Int>` is one object program-wide. This
interning is what keeps the type checker's memory bounded and its comparisons
O(1).
