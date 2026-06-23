namespace std.core

/// The result of a fallible operation: either a value or an error.
/// See docs/design/17-exceptions.md.
public enum Result<out T, out E> {
    Ok(value: T),
    Err(error: E)

    public fn isOk(): Bool {
        return match (this) {
            case Ok(_) => true
            case Err(_) => false
        }
    }

    public fn unwrap(): T {
        return match (this) {
            case Ok(let v) => v
            case Err(let e) => panic("called unwrap on an Err: ${e}")
        }
    }

    public fn unwrapOr(fallback: T): T {
        return match (this) {
            case Ok(let v) => v
            case Err(_) => fallback
        }
    }

    public fn map<U>(transform: (T) -> U): Result<U, E> {
        return match (this) {
            case Ok(let v) => Ok(transform(v))
            case Err(let e) => Err(e)
        }
    }
}

/// Equality comparison.
public interface Equatable {
    public fn equals(other: This): Bool
}

/// Total ordering. Returns negative, zero, or positive.
public interface Comparable {
    public fn compareTo(other: This): Int
}

/// Types that can produce a stable hash.
public interface Hashable ext Equatable {
    public fn hash(): Int
}

/// A lazily evaluated sequence of values.
public interface Sequence<out T> {
    public fn iterator(): Iterator<T>
}

public interface Iterator<out T> {
    public fn next(): T?
}

/// Aborts the program with a message. For unrecoverable invariant violations
/// only; recoverable failures use Result (docs/design/17-exceptions.md).
public fn panic(message: String): Nothing {
    Runtime.panic(message)
}
