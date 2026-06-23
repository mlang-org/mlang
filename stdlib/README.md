# MLang Standard Library

The standard library is written in MLang itself (`*.m`). The compiler provides a
small set of intrinsics and the runtime provides the floor; everything here is
ordinary MLang code. See [docs/design/20-stdlib.md](../docs/design/20-stdlib.md).

## Modules

| Module | Path | Contents |
|--------|------|----------|
| `std.core` | `std/core/` | `Result`, `Option`, `Equatable`/`Comparable`/`Hashable`, `Sequence`, `panic`. |
| `std.io` | `std/io/` | `Console`, readers/writers. |
| `std.collections` | `std/collections/` | `List`/`MutableList`, `Map`/`MutableMap`, sets, queues. |
| `std.string` | `std/string/` | `String`, `StringBuilder`. |
| `std.math` | `std/math/` | constants, numeric functions. |
| `std.time` | `std/time/` | `Instant`, `Duration`. |
| `std.concurrent` | `std/concurrent/` | `Channel`, `Task`, `gather`. |
| `std.json` | `std/json/` | `JsonValue`, parse/stringify. |
| `std.fs` | `std/fs/` | `File`, async I/O. |
| `std.reflect` | `std/reflect/` | `TypeInfo`, `typeOf`. |
| `std.crypto` | `std/crypto/` | hashes, CSPRNG. |
| `std.regex` | `std/regex/` | `Regex`, `Match`. |
| `std.log` | `std/log/` | `Logger`, levels. |
| `std.test` | `std/test/` | assertions for the built-in test framework. |

These modules are the reference surface of the library as described in the
design document. As the compiler backend matures (see
[development phases](../docs/design/37-development-phases.md)), they become the
compiled, shipped standard library.
