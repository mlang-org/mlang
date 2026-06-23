# 35. Example Programs

This chapter is a guided tour of MLang through complete programs. The runnable
sources live in [`examples/`](../../examples/); each is annotated here to show the
language features it exercises. Read top to bottom for a tour from "hello" to a
concurrent server.

## 35.1 Hello world (`examples/hello.m`)

```mlang
namespace examples.hello

import std.io.Console

public fn main() {
    let name: "MLang"
    Console.println("Hello, ${name}!")
}
```

Shows: namespaces, imports, `let` immutable bindings with inferred type, string
interpolation, the `main` entry point.

## 35.2 Values, null safety, control flow (`examples/basics.m`)

Shows: `var` vs `let`, `T?` and the safe-navigation/Elvis operators, `if`/`while`/
`for-in`, ranges, and `match` as an expression with smart-casts. The exhaustive
`match` over a sealed type needs no `default`, and removing a case from the type
turns the match into a compile error - refactoring safety in action (Chapter 8).

## 35.3 Types: classes, structs, enums, interfaces (`examples/types.m`)

Shows: a value-type `struct Vector3` with operators; a `class Account` with
`constructor`/`destructor`, `public`/`private`/`readonly` properties, and methods;
an `enum Rank`; an `interface Shape` with implementers; `abstract class Entity`;
and an `extension` adding `isNumeric()` to `String`. Demonstrates inheritance
(`ext`), interface implementation (`impl`), and that destructors run
deterministically at end of scope (Chapter 13).

## 35.4 Generics and collections (`examples/generics.m`)

Shows: a generic `class Stack<T>`, a generic `class Cache<K: Hashable, V>` with a
bound, generic functions with `where` clauses, variance (`Producer<out T>`), and
the collection pipeline API (`map`/`filter`/`fold`/`sorted`/`groupBy`) over
`List` and `Map`. Demonstrates monomorphization (no boxing) and closure inlining
(Chapters 10, 18).

## 35.5 Error handling (`examples/errors.m`)

Shows: `Result<T, E>`, the `?` propagation operator, `try`/`catch`/`finally`
sugar, custom error enums with exhaustive `catch`, and the distinction between a
recoverable error (returned) and a panic (`assert`, `x!`, out-of-bounds). See
Chapter 17.

## 35.6 Concurrency (`examples/concurrency.m`)

Shows: `async fn`/`await`, `launch` within a structured `scope`, `gather` over a
list of tasks, a `Channel<T>` producer/consumer, `select`, and cooperative
cancellation. Demonstrates that thousands of tasks are cheap (stackless
coroutines, Chapter 16) and that the program leaks nothing on completion because
the scope joins its children (Chapter 15).

## 35.7 A small web service (`examples/webserver.m`)

A few-hundred-line program that ties it together: an `HttpServer` with routing,
JSON request/response via `@Serializable` (reflection, Chapter 19), a
concurrency-safe in-memory store behind a `Mutex`, structured-concurrency request
handling, `Result`-based error handling mapped to HTTP status codes, and
structured logging. This is the "real program" that shows MLang at application
scale.

## 35.8 Systems-level code (`examples/systems.m`)

Shows the low-level end: an `unsafe` block doing raw pointer work behind a safe
API, FFI to a C function (`@extern("c")`, Chapter 30), `wrapping_*` arithmetic,
explicit `&`/`&mut` borrows to avoid ARC, and `@layout(c)` structs. Demonstrates
that MLang reaches down to C-level control when needed, with every such reach
marked `unsafe` and greppable (Chapter 31).

## 35.9 How the examples are used

Every example in `examples/` is compiled (and where it has no external
dependencies, run) by CI, so the tour cannot drift from the language. They double
as: a learning path, a smoke test for the compiler, and a corpus for `mfmt`
idempotence and parser conformance testing (Chapters 23, 34).
