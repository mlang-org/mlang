namespace examples.errors

import std.io.Console
import std.core.Result
import std.fs.File

// Recoverable errors are values; `?` propagates them.
public fn readConfig(path: String): Result<Config, IoError> {
    let text: File.readToString(path)?
    let config: Config.parse(text)?
    return Ok(config)
}

// A custom error enum.
public enum ValidationError {
    Empty,
    TooLong(limit: Int),
    BadChar(at: Int)
}

public fn validate(name: String): Result<String, ValidationError> {
    if (name.isEmpty()) {
        return Err(ValidationError.Empty)
    }
    if (name.length() > 32) {
        return Err(ValidationError.TooLong(32))
    }
    return Ok(name)
}

// try/catch sugar over Result.
public fn run() {
    try {
        let config: readConfig("app.toml").unwrap()
        Console.println("loaded")
    } catch (e: IoError) {
        Console.error("config error: ${e.message}")
    } finally {
        Console.println("done")
    }
}

public fn main() {
    run()
    // Panics are for bugs, not control flow:
    let xs: [1, 2, 3]
    let first: xs[0]
    Console.println("first: ${first}")
}
