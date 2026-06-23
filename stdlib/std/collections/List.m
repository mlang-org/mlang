namespace std.collections

import std.core.{ Sequence, Iterator, Comparable }

/// An immutable, value-semantic, copy-on-write ordered sequence.
/// Mutation of a uniquely-owned buffer happens in place (docs/design/20-stdlib.md).
public class List<out T> impl Sequence<T> {
    private readonly let buffer: Buffer<T>

    public constructor() {
        buffer = Buffer.empty()
    }

    public fn size(): Int {
        return buffer.length
    }

    public fn isEmpty(): Bool {
        return buffer.length == 0
    }

    public fn get(index: Int): T {
        return buffer.at(index)
    }

    public fn map<U>(transform: (T) -> U): List<U> {
        let out: MutableList<U>()
        for (item in this) {
            out.add(transform(item))
        }
        return out.toList()
    }

    public fn filter(predicate: (T) -> Bool): List<T> {
        let out: MutableList<T>()
        for (item in this) {
            if (predicate(item)) {
                out.add(item)
            }
        }
        return out.toList()
    }

    public fn fold<A>(initial: A, combine: (A, T) -> A): A {
        var acc: initial
        for (item in this) {
            acc = combine(acc, item)
        }
        return acc
    }

    public fn any(predicate: (T) -> Bool): Bool {
        for (item in this) {
            if (predicate(item)) {
                return true
            }
        }
        return false
    }

    public fn iterator(): Iterator<T> {
        return buffer.iterator()
    }
}

/// The mutable companion to List.
public class MutableList<T> impl Sequence<T> {
    private var buffer: Buffer<T>

    public constructor() {
        buffer = Buffer.empty()
    }

    public fn add(item: T) {
        buffer = buffer.push(item)
    }

    public fn removeAt(index: Int): T {
        return buffer.removeAt(index)
    }

    public fn size(): Int {
        return buffer.length
    }

    public fn toList(): List<T> {
        return List.fromBuffer(buffer)
    }

    public fn iterator(): Iterator<T> {
        return buffer.iterator()
    }
}
