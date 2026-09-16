# Lightweight Code Syntax Highlighting

This document describes the backend-neutral syntax-token foundation used by dedicated conversation code blocks. The implementation targets Visual C++ 7.1 / Windows Server 2003 and intentionally remains lightweight enough for the Pentium 4 target. The newest tranche remains pending real-target validation.

## Goals

The code presenter needs enough language awareness to make SaaS/LLM code responses easier to scan without turning SalixWeb32 into a compiler front end.

The important separation is:

```text
canonical code characters
        |
        v
CodeSyntaxHighlighter
        |
        v
per-character TextFormat syntax_style
        |
        v
CodeBlockView / platform renderer
```

The source string remains authoritative. Highlighting is presentation metadata only.

## Language identity

`CodeLanguageRegistry` owns canonical fence tokens and their human-readable names. The same registry is used by:

- the native-backed composer language selector,
- Markdown fence serialization,
- `CodeBlockView` language headers,
- `CodeSyntaxHighlighter` language-family selection.

This prevents the composer and renderer from slowly acquiring different alias tables.

Current canonical languages are:

```text
(unlabelled)
c
cpp
csharp
java
python
javascript
typescript
json
bash
rust
lua
html
css
xml
text
```

Unknown fence tokens remain valid and visible; they simply receive generic code formatting until a tokenizer family is available.

## Syntax classes

`TextFormat` now exposes `SyntaxStyle` independently of `CodeStyle`:

```text
syntax_none
syntax_keyword
syntax_string
syntax_comment
syntax_number
syntax_preprocessor
syntax_literal
syntax_tag
```

A character can therefore be both:

```text
code_style   = code_block
syntax_style = syntax_string
```

This is important because `code_style` controls structural behavior such as Courier New and emoticon suppression, while `syntax_style` describes how a token may be presented.

## Current tokenizers

The implementation groups related languages into small tokenizer families rather than maintaining a heavyweight parser for every language.

### C-like family

Used by C, C++, C#, Java, JavaScript, TypeScript, and Rust for the current first pass.

Recognized concepts include:

- common language keywords,
- quoted strings/characters,
- numeric literals,
- `//` line comments,
- `/* ... */` block comments,
- common literal words such as `true`, `false`, `null`, and `nullptr`,
- preprocessor lines for C/C++/C# where applicable.

Language-specific grammar differences are intentionally not fully modeled yet.

### Python

Recognizes Python keywords, quoted strings, numeric literals, `#` comments, and Python literals such as `None`, `True`, and `False`.

### JSON

Recognizes strings, numbers, and literal values. JSON does not gain fake comment semantics.

### Shell

Recognizes a small set of shell control keywords, quoted/backtick strings, numbers, and `#` comments.

### Lua

Recognizes Lua keywords, strings, numbers, `--` line comments, `--[[ ... ]]` long comments, and Lua literals.

### HTML/XML

Recognizes tags, attribute-like identifiers, quoted attribute values, and `<!-- ... -->` comments.

### CSS

Recognizes comments, strings, numbers, and property-like identifiers before `:` as lightweight keyword-style tokens.

## Presentation

The highlighter writes syntax information into `FormattedText`; it never injects escape sequences, markup, or replacement characters into the code body.

The first presentation pass is deliberately conservative for the legacy target:

- code remains Courier New,
- keyword/preprocessor/tag classes receive emphasis,
- comments receive comment-style emphasis,
- selection continues to use normal Windows highlight colors,
- Copy returns only the original source characters,
- syntax metadata never changes caret or selection indexes.

A richer color palette can be layered onto the same `SyntaxStyle` values later without changing parsing, storage, clipboard, or transport contracts.

## Performance model

Highlighting is performed when a `CodeBlockView` receives new code, not on every paint pass. The result is stored as formatted text and reused by measurement and rendering.

The tokenizer is a linear source scan with small static keyword tables. It does not construct an AST, allocate a token object for each lexeme, or invoke external compiler services. This is appropriate for the current Pentium 4 proof while leaving room for more sophisticated providers later.

## Relationship to Markdown and SaaS content

Markdown remains the interchange format. A response such as:

````text
```python
def greet(name):
    print("Hello, " + name)
```
````

is parsed into a code block with language token `python`. The code block then asks `CodeSyntaxHighlighter` for Python presentation metadata.

The same path is used for locally composed code because the composer now emits the selected language token in its Markdown fence. No site-specific presentation format is required.

## Known limits

This first pass is not a compiler, parser generator, or LSP client. Known limits include:

- no semantic type/function resolution,
- no nested/multiline string grammar beyond simple quote scanning,
- no regex-literal parser for JavaScript,
- no template-string interpolation model,
- no Python triple-quoted string model yet,
- no language-specific generic/template grammar,
- no diagnostics or error underlining,
- no symbol lookup or completion,
- no syntax-aware indentation,
- no user-selectable color themes yet.

These are acceptable because the syntax layer is isolated behind a backend-neutral contract and does not contaminate canonical message content.

## Target validation checklist

Before marking this tranche target-validated:

1. Rebuild after reopening the VS2003 solution because the project gained new `.cpp/.h` files.
2. Verify the application launches on Windows Server 2003 SP2 with no new VC7.1 compile/link failures.
3. Submit a Python code block containing `def`, `if`, a string, a number, and a `#` comment.
4. Submit C/C++ code containing a preprocessor line, keyword, string, number, `//` comment, and `/* */` comment.
5. Submit JSON containing strings, numbers, `true`, `false`, and `null`.
6. Submit HTML/XML with a tag, attribute, quoted value, and comment.
7. Verify syntax presentation does not alter the raw characters copied by the code-block Copy button.
8. Verify partial selection/Ctrl+C still copies literal source text.
9. Verify `:)`, `:D`, and `<3` remain literal inside highlighted code.
10. Verify ordinary prose outside code still receives graphical emoticons.
11. Verify long highlighted lines still use the code block's horizontal overflow path.
12. Verify selection and hit-testing remain aligned after horizontal scrolling.
13. Repeat a MiniXP smoke test after Server 2003 passes.
