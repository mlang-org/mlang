namespace std.string

import std.core.{ Comparable, Hashable }

/// An immutable UTF-8 string with value semantics and copy-on-write storage.
/// Short strings use inline storage (docs/design/20-stdlib.md).
public class String impl Comparable, Hashable {
    public fn length(): Int {
        return Intrinsics.utf8Length(this)
    }

    public fn isEmpty(): Bool {
        return length() == 0
    }

    public fn charAt(index: Int): Char {
        return Intrinsics.utf8CharAt(this, index)
    }

    public fn concat(other: String): String {
        return Intrinsics.utf8Concat(this, other)
    }

    public fn substring(start: Int, end: Int): String {
        return Intrinsics.utf8Slice(this, start, end)
    }

    public fn contains(needle: String): Bool {
        return indexOf(needle) >= 0
    }

    public fn indexOf(needle: String): Int {
        return Intrinsics.utf8IndexOf(this, needle)
    }

    public fn between(open: String, close: String): String? {
        let start: indexOf(open)
        if (start < 0) {
            return null
        }
        let from: start + open.length()
        let end: substring(from, length()).indexOf(close)
        if (end < 0) {
            return null
        }
        return substring(from, from + end)
    }

    public fn equals(other: String): Bool {
        return compareTo(other) == 0
    }

    public fn compareTo(other: String): Int {
        return Intrinsics.utf8Compare(this, other)
    }

    public fn hash(): Int {
        return Intrinsics.utf8Hash(this)
    }
}

/// Efficient incremental string construction without repeated allocation.
public class StringBuilder {
    private var buffer: ByteBuffer

    public fn append(text: String): StringBuilder {
        buffer.appendUtf8(text)
        return this
    }

    public fn build(): String {
        return buffer.toString()
    }
}
