# 26. Test Framework

Testing is a first-class, built-in capability, not a third-party add-on. Tests are
written with the `@Test` attribute, discovered automatically, and run by
`mbuild test`.

## 26.1 Writing tests

```mlang
import std.test.{ assert, assertEquals, assertThrows }

@Test
fn loginSucceedsWithValidCredentials() {
    let auth = AuthService()
    let result = auth.login("alice", "correct-horse")
    assert(result.isOk())
    assertEquals(result.unwrap().username, "alice")
}

@Test(name: "rejects empty password")
fn rejectsEmptyPassword() {
    assertThrows<ValidationError>(() => AuthService().login("alice", ""))
}

@Test(tags: ["slow", "integration"])
async fn fetchesRealData() {
    let data = await Api.fetch("/health")
    assertEquals(data.status, 200)
}
```

`@Test` functions can be sync or `async` (the runner drives them on the
scheduler), can carry a display name and tags, and live alongside the code in
`tests/` or in `#[cfg(test)]`-style test modules within a source file.

## 26.2 Discovery via reflection

The runner discovers tests through the attribute/reflection mechanism
(Chapter 19): the compiler retains metadata for `@Test`-annotated functions, and
the test binary enumerates them at startup. No registration macros, no manual
test lists, no naming convention to remember; annotate and it runs.

## 26.3 Assertions and diagnostics

The assertion library captures expression structure so failures are informative:

```
assertEquals failed
  expected: "alice"
  actual:   "alise"
            at tests/auth_test.m:11
```

`assert(cond)` reports the source text of `cond` and the values of its
subexpressions (power-assert style), so a bare `assert(a + b == c)` tells you the
actual values of `a`, `b`, and `c`, not just "assertion failed".

## 26.4 The runner

`mbuild test` builds the test target and runs it with:

- **Parallel execution** by default (tests run on the scheduler; tests that need
  isolation declare it).
- **Filtering** by name substring, tag, or file (`mbuild test auth`,
  `mbuild test --tag slow`).
- **Deterministic seeds** for any randomized test (the seed is printed and can be
  replayed).
- **Output formats**: human (default), JUnit XML, and JSON for CI ingestion.
- **Fail-fast** and **retry** options for flaky-test triage.

## 26.5 Fixtures and lifecycle

`@BeforeEach`/`@AfterEach`/`@BeforeAll`/`@AfterAll` provide setup and teardown.
Because MLang has deterministic destruction (Chapter 13), most fixtures are just
local values whose destructors clean up; the lifecycle hooks exist for shared or
cross-test resources.

## 26.6 Benchmarks and property tests

- `@Bench` functions are measured by `mbuild bench` with warmup, multiple
  iterations, outlier rejection, and stable statistics (median + IQR), so
  benchmark numbers are comparable run-to-run.
- `@Property` tests integrate generative (property-based) testing: declare
  invariants over generated inputs; the runner shrinks failing cases to a minimal
  reproducer. This is the same machinery used to fuzz the compiler itself
  (Chapter 10).

## 26.7 Coverage

`mbuild test --coverage` instruments the build (via the backend's coverage
support) and reports line/branch coverage, with an export for CI dashboards.
Coverage is opt-in because instrumentation perturbs performance and binary size.

## 26.8 Why built-in

A built-in, convention-free test story means every MLang project tests the same
way, CI integration is uniform, and the language can use its own framework to test
the standard library and (via property tests and differential testing) the
compiler. Fragmented third-party test frameworks - the Java/JS experience - impose
a per-project learning cost MLang avoids by shipping one good answer.
