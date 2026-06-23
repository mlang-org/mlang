# 15. Concurrency Model

MLang's concurrency model is **structured concurrency over coroutines**, with
CSP-style channels for communication and a compile-time guarantee of data-race
freedom. The goal: concurrency that is cheap, composable, and safe by default.

## 15.1 Structured concurrency

The central idea: the lifetime of a concurrent task is bounded by a lexical scope.
A task cannot outlive the scope that spawned it; when the scope exits, all tasks
spawned in it are awaited (or cancelled). This makes concurrent code as easy to
reason about as sequential code, and it eliminates leaked tasks.

```mlang
async fn handleRequest(req: Request): Response {
    return scope {                 // a structured-concurrency scope
        let user  = launch fetchUser(req.id)      // child task
        let prefs = launch fetchPrefs(req.id)     // child task
        // scope cannot exit until both children complete or are cancelled
        Response.of(await user, await prefs)
    }   // if either child fails, the other is cancelled, the scope propagates the error
}
```

Cancellation is **cooperative and propagating**: cancelling a scope cancels its
children, which cancel theirs. A cancelled coroutine resumes with a cancellation
signal at its next suspension point and unwinds (running destructors), so
resources are released. This is the Kotlin/Swift structured-concurrency model,
chosen because unstructured "fire and forget" tasks are a leading source of leaks
and lifetime bugs.

## 15.2 Tasks and futures

- `async fn f(): T` returns a `Task<T>`. The body does not run until the task is
  awaited or launched.
- `await t` suspends the current coroutine until `t` completes and yields its
  value (or propagates its error).
- `launch f()` schedules a child task in the current scope and returns a handle.
- `gather(tasks)` awaits a collection concurrently; `select { ... }` waits on the
  first of several operations (a channel receive, a timer, a task completion).

## 15.3 Channels

Channels are the primary way independent tasks communicate, following CSP:

```mlang
let ch: Channel<String>(capacity: 16)

launch {
    for (line in source) { ch.send(line) }   // suspends if full
    ch.close()
}

for (line in ch) {                            // suspends if empty; ends on close
    process(line)
}
```

Channels can be buffered or unbuffered (rendezvous), are multi-producer/
multi-consumer, and integrate with `select`. They transfer **ownership** of the
sent value (it is moved into the channel), which is what makes cross-task sharing
safe without locks: the sender no longer has access after `send`.

## 15.4 Data-race freedom

A data race is two threads accessing the same memory concurrently with at least
one write and no synchronization. MLang rules this out at compile time using the
same ownership machinery as memory safety (Chapter 13):

- **`Send`** marks a type whose ownership may be transferred between threads.
  Moving a value into a `launch`ed task or a channel requires `Send`.
- **`Sync`** marks a type that may be *shared* (`&T`) across threads. Sharing a
  borrow with another task requires `Sync`.
- The aliasing-XOR-mutability rule (Chapter 13) means you can never have a shared
  mutable borrow across tasks. Mutable shared state must go through a type that
  provides synchronization (`Mutex<T>`, `Atomic<T>`, a channel), and those types
  encapsulate the unsafe interior so the user-facing API stays race-free.

`Send`/`Sync` are auto-derived structurally (a struct is `Send` if its fields
are) and can be opted out for types with thread-affinity. The upshot: if it
compiles, it has no data races. This is the Rust guarantee, reused because it is
the only widely-proven static approach.

## 15.5 Memory model

For the `unsafe` and `Atomic<T>` layer that underpins the safe abstractions,
MLang adopts the **C/C++20 memory model**: `relaxed`, `acquire`, `release`,
`acq_rel`, and `seq_cst` orderings, with the same happens-before semantics. Safe
code never names an ordering; it uses `Mutex`, `Channel`, and structured
concurrency, which are documented to provide the appropriate synchronization.
Exposing the standard model (rather than inventing one) means the formal
underpinnings and the hardware mappings are already understood.

## 15.6 Threads vs coroutines

Coroutines are the default unit of concurrency (millions are cheap; Chapter 16).
Real OS threads are available via `Thread.spawn { ... }` for CPU-bound work that
should not share a scheduler worker, and the scheduler itself runs on a pool of
OS threads. Most application code never touches `Thread` directly; it uses
`async`/`launch`/`Channel` and lets the scheduler place work on threads.

## 15.7 Why not "share memory by communicating" only, or locks only

Go shows channels-first concurrency is ergonomic but does not statically prevent
races (Go races are a runtime detector's job). Java/C# show locks are flexible but
race-prone. MLang takes channels and structured concurrency for ergonomics *and*
ownership-based `Send`/`Sync` for static safety, so it gets Go's feel with a
compile-time guarantee Go cannot make.
