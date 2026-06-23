# 27. File Organization

This chapter defines how MLang source is organized: the relationship between
files, namespaces, modules, and visibility.

## 27.1 Files, namespaces, and modules

- A **file** is a `.m` source file. It begins with an optional `namespace`
  declaration and a list of `import`s, then declarations.
- A **namespace** is a logical naming scope (`com.example.project`). Multiple
  files may share a namespace; a namespace may span files and directories.
- A **module** is the unit of compilation and the unit of separate compilation
  (it produces one `.mmi`, Chapter 2). A module is, by default, a package's
  source tree; large packages may split into sub-modules declared in `mango.json`.

The deliberate separation of *namespace* (logical naming) from *module*
(compilation unit) means you can reorganize files without breaking the public
name of a symbol, and you can split a module for build parallelism without
renaming anything.

## 27.2 The file header

```mlang
namespace com.example.banking

import std.collections.{ List, Map }
import std.io.Console
import com.example.banking.model.Account

public class Ledger { ... }
```

- `namespace` (if present) must be the first declaration.
- `import` brings names into scope. Forms: single (`import a.b.C`), grouped
  (`import a.b.{ C, D }`), aliased (`import a.b.C as Acct`), and wildcard
  (`import a.b.*`, discouraged outside scripts). Imports are resolved against the
  dependency graph (Chapter 21) and are order-independent.

## 27.3 Visibility

Five visibility levels, from most to least restrictive:

| Modifier | Visible to |
|----------|------------|
| `private` | the enclosing type (or file, for top-level) |
| `protected` | the type and its subclasses |
| `internal` | the whole module/package |
| `public` | everyone (exported in the `.mmi`) |
| (default) | `internal` |

The default is `internal`, not `public`: a symbol is package-private unless you
opt into exporting it. This makes the public API a deliberate, reviewable surface
(only `public` lands in the `.mmi`) and lets the optimizer treat non-`public`
symbols as closed-world (full devirtualization and inlining, Chapter 10).

## 27.4 One public type per file (a convention, not a rule)

By convention a file is named after its primary public type
(`Account.m` declares `Account`), as in Java/Swift, which makes navigation
predictable. Unlike Java this is a convention, not a compiler requirement; a file
may contain several declarations, and `mfmt`/`mls` do not enforce the file name.

## 27.5 Directory layout maps to namespaces by convention

```
src/
└── com/example/banking/
    ├── Ledger.m            // namespace com.example.banking
    ├── model/
    │   ├── Account.m       // namespace com.example.banking.model
    │   └── Transaction.m
    └── api/
        └── HttpApi.m       // namespace com.example.banking.api
```

The directory tree mirroring the namespace is a strong convention (tooling
scaffolds it and `mls` navigates by it) but the compiler binds by the `namespace`
declaration in the file, not by the path, so a file is never "in the wrong place"
in a way that breaks compilation - only in a way that surprises a reader.

## 27.6 Test placement

Tests live either in a parallel `tests/` tree mirroring `src/`, or inline in a
test module within a source file for white-box tests that need access to
`internal`/`private` members. The build's test target (Chapter 22) compiles both.
