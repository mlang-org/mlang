# 23. Formatter (`mfmt`)

`mfmt` is the canonical code formatter. Like `gofmt`, it is **opinionated and
non-configurable** (beyond a handful of project-wide knobs): there is one MLang
style, so code review never argues about formatting and every file in the
ecosystem looks the same.

## 23.1 Why non-configurable

Configurable formatters fragment a community into style camps and produce churn
when a file moves between projects. A single canonical style means: diffs are
minimal and meaningful, tooling can auto-format on save without surprise, and new
contributors never have to learn a project's bespoke rules. This is the most
important lesson of `gofmt`, and MLang adopts it.

## 23.2 Architecture: it shares the parser

`mfmt` does not have its own parser. It uses `libmlang_frontend` in
full-fidelity mode (Chapter 5): parse to an AST that retains trivia (comments,
blank lines), then pretty-print. Because it is the same parser the compiler uses,
`mfmt` can never accept code the compiler rejects or vice versa, and it preserves
comments exactly.

## 23.3 The algorithm: Wadler/Prettier-style layout

Formatting is a two-step process:

1. **AST -> document IR.** The AST is converted to a layout-algebra document
   built from primitives: `text`, `line` (a space or a newline), `softline`,
   `group` (lay out flat if it fits, else broken), `indent`, and `concat`. This is
   the Wadler "A prettier printer" algebra, as popularized by Prettier.
2. **Document -> text.** A single pass decides, for each `group`, whether it fits
   within the column limit (default 100); if it does, the group is printed flat;
   if not, its `line`s become newlines and its contents indent. The decision is
   made outermost-first so wrapping is stable and predictable.

This approach gives consistent, locally-stable formatting: a small edit causes a
small diff, and long argument lists / chains break in a uniform way.

## 23.4 Style rules (the canonical MLang style)

- 4-space indentation, no tabs; LF line endings; final newline.
- Braces on the same line (`fn f() {`), `else`/`catch` on the closing brace line.
- 100-column soft limit; expressions wrap at the lowest-precedence operator.
- One blank line between declarations; no more than one consecutive blank line.
- Trailing commas in multi-line lists (so adding an item is a one-line diff).
- Imports sorted and grouped (std, third-party, local), de-duplicated.
- Spaces around binary operators; none inside `()`/`[]`; one after `,`/`:`.

## 23.5 Project knobs (the few that exist)

A `mfmt.json` may set only: `lineWidth` (default 100), `lineEnding` (lf/crlf), and
`importGrouping`. There is intentionally no toggle for brace style, indentation
width, or spacing; those are fixed. Keeping the knob set tiny preserves the
"one style" benefit while accommodating the two settings (width, line ending)
that legitimately vary by team and platform.

## 23.6 Modes

- `mfmt .` formats every `.m` file in place.
- `mfmt --check` exits non-zero if any file is not formatted (for CI).
- `mfmt --stdin` formats a stream (for editor "format selection"/"format on
  save", driven by `mls`).
- `mfmt --diff` shows what would change without writing.

## 23.7 Idempotence and stability

`mfmt(mfmt(x)) == mfmt(x)` is a hard invariant, enforced by a fuzz test that
formats random valid programs twice and asserts equality. Idempotence is what
makes "format on save" safe and what keeps formatting out of diffs.
