namespace std.io

import std.string.String

/// Standard input/output access. Buffered; flushed at program shutdown
/// (docs/design/12-runtime.md).
public class Console {
    public static fn print(text: String) {
        Stdout.write(text)
    }

    public static fn println(text: String) {
        Stdout.write(text)
        Stdout.write("\n")
    }

    public static fn error(text: String) {
        Stderr.write(text)
        Stderr.write("\n")
    }

    public static fn readLine(): String? {
        return Stdin.readLine()
    }
}
