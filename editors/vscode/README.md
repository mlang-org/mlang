# MLang Language Support

Official Visual Studio Code extension for the [MLang](https://github.com/mlang-org/mlang)
programming language.

## Features

- Syntax highlighting for `.m` files (TextMate grammar, used for first paint).
- Semantic features via the `mls` language server: diagnostics, completion,
  hover, go-to-definition, rename, and formatting (as `mls` matures, see
  [docs/design/24-mls.md](../../docs/design/24-mls.md)).
- Bracket matching, auto-closing, and comment toggling.

## Requirements

The `mls` language server must be on your `PATH`, or set `mlang.server.path` in
your settings. Build it from the repository with `cmake --build` (the `mls`
target).

## Settings

| Setting | Default | Description |
|---------|---------|-------------|
| `mlang.server.path` | `mls` | Path to the `mls` executable. |
| `mlang.trace.server` | `off` | LSP trace verbosity. |

## Development

This is the reference LSP client. `src/extension.js` launches `mls` over stdio;
the same server works with any LSP-capable editor.
