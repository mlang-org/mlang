namespace std.test

/// Assertions for the built-in test framework. Tests are @Test-annotated
/// functions discovered via reflection (docs/design/26-test-framework.md).

public fn assert(condition: Bool, message: String) {
    if (!condition) {
        TestRuntime.fail(message)
    }
}

public fn assertEquals<T: Equatable>(actual: T, expected: T) {
    if (!actual.equals(expected)) {
        TestRuntime.fail("expected ${expected}, got ${actual}")
    }
}

public fn assertThrows<E>(body: () -> Nothing) {
    match (TestRuntime.capture(body)) {
        case Caught(let e: E) => return
        case Caught(_) => TestRuntime.fail("threw the wrong error type")
        case NoThrow => TestRuntime.fail("expected an error but none was thrown")
    }
}
