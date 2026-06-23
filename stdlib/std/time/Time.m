namespace std.time

/// A monotonic point in time, nanosecond resolution.
public struct Instant impl Comparable {
    private readonly let nanos: Int64

    public static fn now(): Instant {
        return Instant(Clock.monotonicNanos())
    }

    public fn elapsedSince(earlier: Instant): Duration {
        return Duration(nanos - earlier.nanos)
    }

    public fn compareTo(other: Instant): Int {
        return nanos < other.nanos ? -1 : (nanos > other.nanos ? 1 : 0)
    }
}

/// A span of time.
public struct Duration {
    public readonly let nanos: Int64

    public fn millis(): Int64 {
        return nanos / 1000000
    }

    public fn seconds(): Float {
        return (nanos as Float) / 1000000000.0
    }
}
