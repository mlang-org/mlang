namespace std.collections

import std.core.{ Hashable, Sequence, Iterator }

/// An immutable hash map with open-addressing storage chosen for cache
/// behavior (docs/design/20-stdlib.md, 32-performance.md).
public class Map<K: Hashable, out V> {
    private readonly let table: HashTable<K, V>

    public fn size(): Int {
        return table.count
    }

    public fn get(key: K): V? {
        return table.lookup(key)
    }

    public fn contains(key: K): Bool {
        return table.lookup(key) != null
    }

    public fn keys(): Sequence<K> {
        return table.keys()
    }

    public fn filter(predicate: (K, V) -> Bool): Map<K, V> {
        let out: MutableMap<K, V>()
        for (key in keys()) {
            let value: get(key)!
            if (predicate(key, value)) {
                out.put(key, value)
            }
        }
        return out.toMap()
    }
}

public class MutableMap<K: Hashable, V> {
    private var table: HashTable<K, V>

    public fn put(key: K, value: V) {
        table = table.insert(key, value)
    }

    public fn remove(key: K): V? {
        return table.remove(key)
    }

    public fn toMap(): Map<K, V> {
        return Map.fromTable(table)
    }
}
