# 17. Exception System

MLang distinguishes two failure modes and gives each its own mechanism:
**recoverable errors** (a file is missing, input is malformed) are *values*;
**unrecoverable bugs** (a broken invariant, an out-of-bounds index) are *panics*.
Conflating these is the source of much pain in other languages; separating them
makes both clearer.

## 17.1 Recoverable errors are values

A function that can fail returns a `Result<T, E>` (or uses the `throws` sugar over
it). Errors are ordinary values, propagated explicitly, so the type signature
tells the truth about what can fail.

```mlang
fn readConfig(path: String): Result<Config, IoError> {
    let text = File.readToString(path)?    // ? propagates the error
    let cfg  = Config.parse(text)?
    return Ok(cfg)
}
```

The `?` operator is the ergonomic core: on `Ok(v)` it unwraps to `v`; on `Err(e)`
it returns early, converting `e` to the function's error type via a `From`
conversion. This gives the brevity of exceptions with the honesty of explicit
returns: every fallible call is marked with `?`, so error paths are visible.

## 17.2 The `try`/`catch` sugar

For code that prefers exception-style flow, `try`/`catch`/`finally` is sugar over
`Result` and propagation:

```mlang
try {
    let cfg = readConfig("app.toml")
    start(cfg)
} catch (e: IoError) {
    Console.error("config error: ${e.message}")
} finally {
    cleanup()
}
```

A `throw e` is sugar for returning `Err(e)`; a `try` block where a `?`-style
failure occurs jumps to the matching `catch`. `finally` runs on every exit path.
Because this is sugar over values, there is no separate "exception type
hierarchy" the way Java has; a thrown thing is just an error value, and `catch`
arms are pattern matches on its type.

## 17.3 Panics are for bugs

A panic signals a violated invariant: a failed assertion, an out-of-bounds index,
integer overflow in checked mode, a `null` assertion (`x!`) that was wrong,
arithmetic by zero. Panics are not meant to be caught for control flow; they are
meant to crash loudly with a precise message and a backtrace.

```mlang
let x = arr[i]          // panics with index + length if out of bounds
let y = maybe!          // panics if maybe was null
assert(balance >= 0, "balance went negative")
```

By default a panic **unwinds** the current task: it runs destructors up the stack
(releasing resources) and terminates the task; in a structured-concurrency scope
it cancels siblings and propagates. The program can choose a `panic = abort`
policy (smaller binaries, no unwinder) where a panic immediately aborts the
process. A panic can be *caught at a task boundary* (`catchUnwind`) for
supervisor-style isolation (e.g. a request handler that should not take down the
server), but this is a deliberate boundary, not everyday flow.

## 17.4 Zero-cost unwinding

MLang uses **table-based (zero-cost) unwinding**: the happy path has no overhead
(no setjmp, no push/pop of handler frames). When a panic or error-unwind occurs,
the runtime consults side tables (DWARF CFI / `.eh_frame` on ELF, the SEH tables
on Windows) to find destructors and handlers and walks the stack. "Zero-cost"
means the cost is zero when nothing fails and proportional to the unwind only when
something does, which is the right trade because failures are rare.

The compiler emits `invoke`/landing-pad pairs (Chapter 11) only around calls that
can unwind; calls proven non-throwing (most of them, since errors are usually
values) emit plain calls. This keeps the unwinding surface small.

## 17.5 Error-handling rules enforced by the compiler

- A `Result` value must be used: ignoring it is a warning (`must_use`), so silent
  error-dropping is hard to do by accident.
- `?` is legal only where the enclosing function's return type can carry the
  error (a `Result`/`throws` function), checked in semantic analysis (Chapter 8).
- `catch` arms over a sealed error enum are checked for exhaustiveness.
- A function may declare it does not error; calling a fallible function inside it
  without handling the error is then a compile error.

## 17.6 Why values-plus-panics rather than classic exceptions

Classic unchecked exceptions (Java `RuntimeException`, C#, Python) hide control
flow: any call might throw, and the type system does not say which. This causes
"forgot to handle it" production failures. Pure checked exceptions (Java's
`throws`) are honest but notoriously noisy. MLang's model:

- makes the fallible/infallible distinction *visible at every call site* (`?`),
- keeps signatures honest (the error type is in the return type),
- reserves stack unwinding for genuine bugs (panics), where its cost and
  loudness are appropriate,
- and still offers `try`/`catch` ergonomics for those who want them, as sugar.

This is the Rust/Swift consensus (errors as values) with a friendlier surface,
which is the right balance for a language that prizes both safety and readability.
