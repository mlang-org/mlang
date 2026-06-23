namespace std.concurrent

/// A CSP-style channel. Sending transfers ownership of the value, so cross-task
/// sharing is data-race free without locks (docs/design/15-concurrency.md).
public class Channel<T> {
    private readonly let queue: BoundedQueue<T>
    private var closed: Bool

    public constructor(capacity: Int) {
        queue = BoundedQueue(capacity)
        closed = false
    }

    /// Suspends if the channel is full.
    public async fn send(value: T) {
        if (closed) {
            panic("send on a closed channel")
        }
        await queue.put(value)
    }

    /// Suspends if empty; returns null once the channel is closed and drained.
    public async fn receive(): T? {
        return await queue.take()
    }

    public fn close() {
        closed = true
        queue.wakeAll()
    }
}

/// Awaits all tasks concurrently and collects their results.
public async fn gather<T>(tasks: List<Task<T>>): List<T> {
    let out: MutableList<T>()
    for (task in tasks) {
        out.add(await task)
    }
    return out.toList()
}
