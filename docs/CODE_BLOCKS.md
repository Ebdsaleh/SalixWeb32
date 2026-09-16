# Dedicated Conversation Code Blocks

This document records the block-composed conversation tranche that follows the wrapped rich conversation viewport. The implementation is intentionally compatible with the Visual C++ 7.1 / Windows Server 2003 target and remains pending target validation until rebuilt and exercised on the Pentium 4.

## Goal

Fenced Markdown code must no longer be flattened into the same `Label` used for surrounding prose. A single canonical message may contain ordinary text, one or more code blocks, and more ordinary text after the code while the conversation presenter gives each block an appropriate component.

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

The ASCII borders above describe structure only. Actual Win32 presentation uses framework panels, labels, buttons, and scrollbars.

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

## Language label

The opening Markdown fence can identify a language:

````text
```python
...
```
````

The first info token becomes the code-block language label. A small display-name mapper currently recognizes common aliases such as:

- `c` -> `C`
- `cpp`, `c++`, `cxx` -> `C++`
- `python`, `py` -> `Python`
- `javascript`, `js` -> `JavaScript`
- `typescript`, `ts` -> `TypeScript`
- `csharp`, `cs`, `c#` -> `C#`
- `bash`, `sh`, `shell` -> `Shell`
- `html`, `css`, `json`, `xml`, `lua`, `rust`, `java`
- `text`, `plaintext`, `txt` -> `Plain text`

Unknown identifiers remain visible rather than being discarded. An empty info string displays `Code`.

Composer code mode currently emits an unlabelled fenced block, so local code-mode submissions display `Code` until a composer language selector is introduced. SaaS Markdown containing a language token can display it immediately.

## Copy button

The header Copy button copies the complete raw code body to the backend-neutral clipboard as plain text.

It intentionally excludes:

- the Markdown fences,
- the language label,
- the message role header,
- surrounding prose.

The Win32 application host now supplies its existing `Win32Clipboard` through pointer-down/up framework events as well as keyboard events, allowing pointer-triggered framework controls to invoke clipboard actions without importing Win32 APIs into `CodeBlockView`.

Normal selectable-label `Ctrl+C` continues to work inside the code body for partial selections.

## Horizontal overflow

Code blocks deliberately do **not** soft-wrap. Source-code structure and indentation must remain intact.

If the longest code line is wider than the visible body, `CodeBlockView` exposes its own horizontal scrollbar and shifts only the code viewport. The outer `ConversationView` retains independent vertical pixel scrolling.

For this tranche, the per-code-block scrollbar intentionally uses the framework-rendered scrollbar fallback rather than a native child HWND. A native Win32 `SCROLLBAR` child is not safely clipped by the GDI clip region of a vertically scrolled conversation viewport; attaching nested native peers before defining an HWND clipping/parenting policy could allow the control to bleed outside the conversation surface. The main conversation and composer scrollbars remain native Win32 controls.

The `CodeBlockView` keeps the native-peer attachment hooks so this can be revisited after the nested-native-control clipping policy is designed.

## Selection and hit-testing

The code body remains a selectable `Label`:

- mouse selection works against literal source characters,
- Ctrl+C copies selected code,
- code aliases such as `:)` remain literal and therefore have ordinary code hit-testing,
- horizontal scrolling moves presentation only; it does not rewrite the code text or selection state.

## Current limits

This tranche does not yet provide:

- syntax highlighting,
- a composer-side language selector,
- line numbers,
- code folding,
- nested native HWND scrollbars inside the conversation viewport,
- a full CommonMark block parser,
- tilde fences,
- syntax-aware indentation or completion.

These can be added without reverting the block-composed conversation model.

## Target validation checklist

Before marking this tranche validated:

1. Rebuild with Visual C++ 7.1 on the Pentium 4.
2. Send a normal one-line message and confirm compact `You: message` presentation still works.
3. Send a mixed Markdown message containing prose, a fenced code block, and prose after the fence.
4. Verify `You:`/`Remote:` is shown as a separate header for the mixed message.
5. Verify the code block has one coherent bordered/background container rather than disconnected gray line bands.
6. Verify the header shows `Python`, `C`, `C++`, or the supplied language token.
7. Click Copy and paste into Notepad; verify only the raw code body is copied.
8. Select part of the code manually and use Ctrl+C; verify partial selection copy still works.
9. Put `:)`, `:D`, and `<3` inside code and verify they remain literal.
10. Put the same aliases in prose outside code and verify graphical emoticons still render.
11. Send a code line wider than the block and exercise the code block horizontal scrollbar.
12. While horizontally scrolled, select/copy code and verify source positions remain correct.
13. Resize the main window and verify prose reflows while code remains non-wrapped.
14. Scroll the outer conversation vertically through a partially visible code block and verify clipping remains clean.
15. Exercise multiple code blocks in one message.
16. Repeat the build/runtime smoke test under MiniXP after Server 2003 validation.
