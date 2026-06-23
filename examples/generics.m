namespace examples.generics

import std.io.Console
import std.collections.{ List, Map }
import std.core.Comparable

// A generic class.
public class Stack<T> {
    private var items: MutableList<T>

    public constructor() {
        items = MutableList()
    }

    public fn push(item: T) {
        items.add(item)
    }

    public fn pop(): T? {
        if (items.size() == 0) {
            return null
        }
        return items.removeAt(items.size() - 1)
    }
}

// A generic class with a constraint.
public class Cache<K: Hashable, V> {
    private var store: MutableMap<K, V>

    public fn get(key: K): V? {
        return store.get(key)
    }

    public fn put(key: K, value: V) {
        store.put(key, value)
    }
}

// A generic function with a where clause.
public fn clamp<T>(value: T, lo: T, hi: T): T where T: Comparable {
    if (value.compareTo(lo) < 0) {
        return lo
    }
    if (value.compareTo(hi) > 0) {
        return hi
    }
    return value
}

public fn main() {
    let scores: ["alice": 90, "bob": 75, "carol": 88]
    let passing: scores.filter((name, score) => score >= 80)
    Console.println("passing: ${passing.size()}")
}
