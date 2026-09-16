# Dedicated Conversation Code Blocks

This document records the block-composed conversation presentation built above the wrapped rich conversation viewport. The dedicated container has compiled and rendered on the real Visual C++ 7.1 / Windows Server 2003 Pentium 4 target; exhaustive interaction coverage and MiniXP smoke validation remain pending for the newest refinements.

## Goal

Fenced Markdown code must not be flattened into the same `Label` used for surrounding prose. A single canonical message may contain ordinary text, one or more code blocks, and more ordinary text after the code while the conversation presenter gives each block an appropriate component.

The intended presentation is:

```text
You:

Hello from Pentium 4

+--------------------------------------------+
| Python                               Copy |
+--------------------------------------------+
| import os                                  |
|                                            |
| def main():                                |
|     print("I want MSN Messenger back!<3") |
|                                            |
| if __name__ == "__main__":                |
|     main()                                 |
+--------------------------------------------+

Normal prose continues here :D
```

The ASCII borders above describe structure only. Actual Win32 presentation uses framework panels, labels, buttons, text-format semantics, and scrollbars.

## Canonical source remains authoritative

The original `FormattedText` message remains stored by `ConversationView::MessageEntry` and is not rewritten into presentation widgets.

For a canonical message such as:

````text
Hello from Pentium 4

```python
print(":D")
```

Back to normal :D
````

presentation may be split into three components, but the retained message source remains the original Markdown.

This separation is important for future:

- transport to and from SaaS providers,
- message persistence,
- Copy Markdown,
- re-rendering after presentation changes,
- diagnostics,
- richer Markdown support.

## Block parsing

`framework/MarkdownBlockParser` performs lightweight block segmentation above the platform renderer.

Current block types are:

```text
block_text
block_code
```

For fenced code, the parser:

- recognizes triple-backtick opening and closing fences,
- removes the fences from presentation content,
- captures the first info-string token after the opening fence as the language identifier,
- preserves the code body as canonical literal text,
- preserves per-character source formatting while slicing `FormattedText`,
- leaves unmatched/incomplete fences in ordinary text rather than inventing a code block.

This is a deliberately small Markdown block parser, not a full CommonMark parser.

## Message composition

`ConversationMessageView` is the presentation root for one conversation entry.

```text
ConversationView
    |
    +-- ConversationMessageView
            |
            +-- role header (`You:`, `Remote:`, `System:`)
            +-- wrapped text Label
            +-- CodeBlockView
            +-- wrapped text Label
            +-- ...
```

Ordinary short single-line messages retain the compact form:

```text
You: Hello :D
```

Messages with block structure use a separate role header so lists, code and other blocks align beneath the speaker identity.

## CodeBlockView

`CodeBlockView` is a dedicated composite component rather than a special paint case inside `ConversationView`.

It owns:

```text
header Panel
    +-- language Label
    +-- Copy Button
body Panel
    +-- selectable literal code Label
horizontal ScrollBar
```

The body uses `TextFormat::code_block`, which means the Win32 text painter uses Courier New and suppresses graphical emoticon substitution.

Therefore this remains literal code:

```cpp
const char* face = ":D";
```

while ordinary message text outside the block can still display the graphical classic emoticon for `:D`.

## Shared code-language registry

Language identity is centralized in `framework/CodeLanguageRegistry` rather than duplicated between the composer and conversation renderer.

The registry separates canonical Markdown tokens from UI display names. Examples include:

```text
c           -> C
cpp         -> C++
csharp      -> C#
python      -> Python
javascript  -> JavaScript
typescript  -> TypeScript
bash        -> Shell
rust        -> Rust
text        -> Plain text
```

It also canonicalizes familiar aliases such as `c++`, `cxx`, `py`, `js`, `ts`, `cs`, `c#`, `sh`, `shell`, `rs`, `plaintext`, and `txt`.

An empty token displays `Code`. Unknown tokens remain visible rather than being discarded, which matters for SaaS Markdown containing languages we have not specialized yet.

The composer now exposes the same registry through a native-backed language combo box. Choosing `Python`, for example, causes code ranges to be serialized with a `python` fenced-Markdown info token. The parser passes that token back into `CodeBlockView`, so local and remote Markdown follow the same presentation path.

## Lightweight syntax semantics

`framework/CodeSyntaxHighlighter` performs a deliberately lightweight tokenizer before the code body is handed to `Label`.

It never alters `source_code`. Instead it writes syntax metadata into the per-character `TextFormat` array. Current semantic classes are:

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

The first presentation pass uses those semantics conservatively: keywords/preprocessor/tag tokens receive emphasis and comments receive comment-style emphasis while all code retains the same canonical characters, monospace font, selection indexes, and clipboard output.

Current language families include C-like languages, Python, JSON, Shell, Lua, HTML/XML, and CSS. Unknown/plain-text fences simply retain ordinary code formatting without speculative tokenization.

This is intentionally much smaller than an IDE parser. It does not attempt semantic type resolution, AST construction, diagnostics, completion, or full grammar correctness. The important architecture is that syntax identity now exists above the Win32 renderer and can later drive richer palettes without changing the message model.

See `docs/SYNTAX_HIGHLIGHTING.md` for the detailed contract.

## Copy button

The header Copy button copies the complete raw code body to the backend-neutral clipboard as plain text.

It intentionally excludes:

- the Markdown fences,
- the language label,
- syntax-format metadata,
- the message role header,
- surrounding prose.

The Win32 application host supplies its existing `Win32Clipboard` through pointer-down/up framework events as well as keyboard events, allowing pointer-triggered framework controls to invoke clipboard actions without importing Win32 APIs into `CodeBlockView`.

Normal selectable-label `Ctrl+C` continues to work inside the code body for partial selections.

## Horizontal overflow

Code blocks deliberately do **not** soft-wrap. Source-code structure and indentation must remain intact.

If the longest code line is wider than the visible body, `CodeBlockView` exposes its own horizontal scrollbar and shifts only the code viewport. The outer `ConversationView` retains independent vertical pixel scrolling.

For now, the per-code-block scrollbar intentionally uses the framework-rendered scrollbar fallback rather than a native child HWND. A native Win32 `SCROLLBAR` child is not safely clipped by the GDI clip region of a vertically scrolled conversation viewport; attaching nested native peers before defining an HWND clipping/parenting policy could allow the control to bleed outside the conversation surface. The main conversation and composer scrollbars remain native Win32 controls.

The `CodeBlockView` keeps the native-peer attachment hooks so this can be revisited after the nested-native-control clipping policy is designed.

## Selection and hit-testing

The code body remains a selectable `Label`:

- mouse selection works against literal source characters,
- Ctrl+C copies selected code,
- code aliases such as `:)` remain literal and therefore have ordinary code hit-testing,
- syntax metadata does not alter source indexes,
- horizontal scrolling moves presentation only; it does not rewrite the code text or selection state.

## Current limits

The current implementation does not yet provide:

- a full syntax grammar or semantic parser,
- line numbers,
- code folding,
- per-code-block language selection inside one rich composer draft,
- nested native HWND scrollbars inside the conversation viewport,
- a full CommonMark block parser,
- tilde fences,
- syntax-aware indentation or completion,
- compiler/LSP diagnostics.

These can be added without reverting the block-composed conversation model.

## Target validation checklist

Before marking the newest code-language/syntax tranche validated:

1. Rebuild with Visual C++ 7.1 on the Pentium 4.
2. Verify the native language selector is disabled with code mode off and enabled with code mode on.
3. Send Python, C++, JavaScript, JSON, and plain/unlabelled code and confirm the expected code-block header.
4. Verify the generated Markdown contains the expected canonical fence token.
5. Send a mixed message containing prose, a code range, and prose after it; verify distinct blocks remain stable.
6. Verify the code block has one coherent bordered/background container rather than disconnected gray line bands.
7. Click Copy and paste into Notepad; verify only raw code body characters are copied.
8. Select part of the code manually and use Ctrl+C; verify partial selection copy still works.
9. Put `:)`, `:D`, and `<3` inside code and verify they remain literal.
10. Put the same aliases in prose outside code and verify graphical emoticons still render.
11. Exercise representative keywords, strings, comments, numbers, literals, and preprocessor/tag tokens and verify syntax emphasis does not alter source characters.
12. Send a code line wider than the block and exercise the code block horizontal scrollbar.
13. While horizontally scrolled, select/copy code and verify source positions remain correct.
14. Resize the main window and verify prose reflows while code remains non-wrapped.
15. Scroll the outer conversation vertically through a partially visible code block and verify clipping remains clean.
16. Exercise multiple code blocks in one message.
17. Confirm composer native scrollbars remain inactive during click/drag-selection below overflow thresholds and activate only for real overflow.
18. Repeat the build/runtime smoke test under MiniXP after Server 2003 validation.
