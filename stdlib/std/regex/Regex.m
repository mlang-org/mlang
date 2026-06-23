namespace std.regex

import std.core.Result
import std.collections.List

/// A compiled regular expression.
public class Regex {
    private readonly let program: Program

    public static fn compile(pattern: String): Result<Regex, RegexError> {
        return RegexCompiler.compile(pattern)
    }

    public fn matches(input: String): Bool {
        return program.search(input) != null
    }

    public fn findAll(input: String): List<Match> {
        return program.searchAll(input)
    }
}

public struct Match {
    public readonly let text: String
    public readonly let start: Int
    public readonly let end: Int
}

public struct RegexError {
    public readonly let message: String
}
