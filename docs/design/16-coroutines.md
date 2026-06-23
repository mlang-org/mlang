# 16. Coroutine System

Coroutines are how MLang makes concurrency cheap. An `async fn` is compiled into a
**stackless state machine**: suspending does not park an OS thread, it saves a
small struct. This is what lets a program have millions of in-flight tasks.

## 16.1 Stackless, not stackful

Two designs exist for coroutines:

- **Stackful** (goroutines, fibers): each coroutine owns a (growable) stack.
  Simple to implement and call arbitrary code across suspension, but each
  coroutine costs kilobytes and switching touches the stack.
- **Stackless** (Rust/C#/Kotlin `async`): the compiler transforms an `async`
  function into a state machine whose locals live in a heap-allocated frame only
  as large as needed. Suspension is saving an integer state and returning.

MLang uses **stackless** coroutines. The cost of a suspended task is the size of
its live locals (often tens of bytes), not a full stack, so millions of tasks fit
in memory. The trade-off (you can only suspend inside `async` code, not across an
arbitrary native call) is acceptable and matches the structured model.

## 16.2 The state-machine transform

The compiler lowers an `async fn` to a struct plus a `resume` function. Each
`await` becomes a suspension point: a state number, with the live locals saved in
the frame. Conceptually:

```mlang
async fn loadUser(id: Int): User {
    let row  = await db.query(id)       // suspend point 1
    let prefs = await cache.get(id)     // suspend point 2
    return User.from(row, prefs)
}
```

lowers to (sketch):

```
struct loadUser$Frame {
    state: i32
    id: Int
    row: Row?        // live across suspend 1->2
    awaitable: *Awaitable
    // ... resume continuation, scheduler hooks
}

fn loadUser$resume(frame): Poll<User> {
    switch frame.state {
      0: start db.query(frame.id); frame.state = 1; return Pending
      1: frame.row = result; start cache.get(frame.id); frame.state = 2; return Pending
      2: return Ready(User.from(frame.row, result))
    }
}
```

The frame is allocated once when the task starts (and freed when it completes),
and `resume` is driven by the scheduler. Only locals that are live across a
suspension point are stored in the frame; the rest stay in registers/stack during
a single resume, so the frame is minimal.

## 16.3 Awaitables

`await` works on any type implementing `Awaitable`: `Task<T>`, `Future<T>`,
channel operations, timers, and I/O handles. `Awaitable` exposes `poll(waker)`:
the scheduler polls; if not ready, the awaitable stores the `waker` and returns
`Pending`; when the underlying event fires (I/O completion, timer, channel send),
it invokes the `waker`, which reschedules the parent coroutine. This is the
poll/waker model (as in Rust's futures), chosen because it composes without
allocation per combinator and integrates cleanly with the reactor.

## 16.4 The scheduler integration

The runtime scheduler (Chapter 12) owns a pool of OS-thread workers and drives
ready coroutines. A coroutine that returns `Pending` is parked (its waker held by
the awaitable); a coroutine that returns `Ready` completes and wakes its awaiter.
Work-stealing balances load; the reactor wakes coroutines blocked on I/O. Because
the scheduler is M:N, a blocking syscall is funneled through the reactor or a
dedicated blocking pool so it never stalls a worker.

## 16.5 Generators (`yield`)

The same transform powers synchronous generators:

```mlang
fn fibonacci(): Sequence<Int> {
    var a = 0; var b = 1
    while (true) {
        yield a
        let next = a + b; a = b; b = next
    }
}
```

`yield` is a suspension point that produces a value to the consumer and resumes on
the next `next()`. Generators share the state-machine machinery with `async`; the
difference is the driver (a `for` loop pulling values vs. the scheduler).

## 16.6 Cancellation and cleanup

Cancellation (Chapter 15) is delivered at suspension points: a cancelled
coroutine's next `resume` observes the cancel flag and unwinds the frame, running
destructors for live locals (RAII holds across `await`). This is why structured
concurrency can promise that cancelling a scope releases its children's resources.

## 16.7 Why this model

`async`/`await` with stackless coroutines and a poll/waker scheduler is the
design that has won across Rust, C#, and (effectively) Kotlin, because it gives:
cheap tasks, no function-coloring surprises beyond the `async` marker, no
per-combinator allocation, and a clean cancellation story. MLang adopts it rather
than inventing a new concurrency primitive.
