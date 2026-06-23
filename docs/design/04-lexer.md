# 4. Lexer

The lexer turns a byte stream into a token stream. It is hand-written (not
generated) for speed, precise error spans, and the ability to recover gracefully.

## 4.1 Source model

- Input is **UTF-8**. The lexer validates encoding as it goes and reports invalid
  byte sequences as diagnostics rather than crashing.
- A `SourceManager` owns all loaded files and assigns each a `FileId`. Every
  byte in the program has a global `SourceLoc` (a 32-bit offset into a virtual
  concatenation of all files), which is cheap to store on every token and AST
  node. Line/column are computed lazily from a per-file line-start table only
  when a diagnostic is rendered.
- Identifiers are Unicode (UAX #31): a program may use non-ASCII identifiers, but
  keywords are ASCII. Normalization is NFC; two identifiers that are not
  byte-equal after NFC are different.

## 4.2 Tokens

A token is a 16-byte value: a `TokenKind` (one byte), flags (e.g. "preceded by
newline", used by automatic semicolon handling), and a `SourceRange` (start +
length). Token text is not copied; it is a view into the source buffer.
Identifiers and keywords are interned into a global `Symbol` (a 32-bit id) so
that later comparisons are integer comparisons.

Token categories:

- **Keywords**: `namespace import let var fn class struct interface enum
  abstract extension impl ext constructor destructor public private protected
  internal static async await launch return if else match case default for while
  do break continue try catch finally throw new this super null true false in is
  as readonly override virtual unsafe yield where`.
- **Identifiers**: UAX #31 identifiers that are not keywords.
- **Literals**: integer, float, string, raw string, interpolated string,
  character, boolean, null.
- **Punctuation/operators**: `+ - * / % ** & | ^ ~ << >> && || ! == != < > <= >=
  = += -= *= /= %= &&= ||= ?: ?. ?? . , ; : :: -> => ( ) [ ] { } < > @ ?`.
- **Trivia**: whitespace, line comments (`//`), block comments (`/* */`, nesting
  allowed), and doc comments (`///`). Trivia is attached to the following token
  so that `mfmt` and `mls` can reproduce or reason about it; the parser ignores
  it.

## 4.3 Numeric literals

```
decimal      123     1_000_000
hex          0xFF    0xDE_AD_BE_EF
octal        0o755
binary       0b1010_0101
float        3.14    6.022e23    1_0.5
typed suffix 10i32   255u8       3.5f32    100L (i64)
```

Underscores are digit separators and are stripped. Suffixes pin the type;
without a suffix, integer literals default to `int` (i64) and float literals to
`float` (f64), subject to type inference (Chapter 7). The lexer records the radix
and the raw digits; conversion and range checking happen in semantic analysis so
that `255u8` vs `256u8` is a typed diagnostic, not a lexer error.

## 4.4 String literals and interpolation

```mlang
let plain:  "hello\n"
let interp: "Hello, ${name}! You have ${count + 1} messages."
let raw:    r"C:\Users\me\nfile"          // no escapes
let rawhash: r#"He said "hi" and \n"#     // choose your own delimiter
let multi:  """
            line one
            line two
            """                            // indentation stripped to the close
```

String interpolation is lexed as a sequence of tokens, not a single string: the
lexer emits `StringStart`, then alternating `StringChunk` and a full nested
token stream for each `${ ... }` expression, then `StringEnd`. The parser
reassembles these into a `StringInterp` AST node. This keeps the grammar regular
(interpolated expressions are ordinary expressions) and gives exact spans for
errors inside `${ ... }`.

## 4.5 Automatic semicolon handling

MLang does not require semicolons. The lexer marks every token that begins a line
with a `StartsLine` flag. The parser treats a newline as a statement terminator
*unless* the line is in a clearly continued state (the previous token is a binary
operator, an open bracket, a comma, or `.`/`?.`, or the next token cannot begin a
statement). Explicit `;` is always accepted. This is the Go/Swift approach:
predictable, with a small, documented continuation rule rather than full JS-style
ASI guesswork.

## 4.6 Error recovery

The lexer never throws. On an unexpected byte it emits a `TokenKind::Error` token
carrying the offending range and a diagnostic, then skips to the next plausible
boundary (whitespace or a known delimiter). Unterminated strings and block
comments are reported at end-of-file with the opening location noted. This lets
the parser keep going and the user see all their lexical errors at once.

## 4.7 Performance

- Single pass, no backtracking. The maximal-munch decisions (`<` vs `<<` vs
  `<=` vs `<<=`) are a fixed small lookahead.
- The classifier for the first byte of a token is a 256-entry lookup table.
- Identifier interning uses an open-addressing hash map with FNV-1a over the raw
  bytes; the common case (already-interned keyword/identifier) is one probe.
- Target: > 20 million tokens/second/core on commodity hardware, which keeps
  lexing well under 5% of total frontend time.
