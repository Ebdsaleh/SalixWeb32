# Markdown Conversation Presentation

This document records the native Markdown presentation layer for SalixWeb32 conversation history. The initial Markdown/list/code presentation has built successfully on the real Windows Server 2003 Pentium 4 target. Dedicated mixed-message code-block containers are the current pending target-validation tranche.

## Goal

Conversation messages keep canonical source text while the presentation layer derives framework components and `FormattedText` for display. This lets local and future remote/SaaS messages use Markdown without replacing the portable message payload.

A canonical list such as:

```text
1. one
2. two
3. three
```

is presented as a message block beneath the role header rather than placing the first list item after `You:`:

```text
You:

1. one
2. two
3. three
```

Single-line ordinary messages retain the compact form:

```text
You: Hello from Pentium 4 :D
```

## Architecture

Markdown presentation now has two complementary layers.

### Inline / text formatting

`framework/MarkdownFormatter` remains backend-neutral and converts canonical text plus existing per-character `TextFormat` into presentation `FormattedText`.

It handles inline emphasis, headings, list/quote presentation, inline code semantics, and other text-oriented Markdown behavior.

### Block segmentation

`framework/MarkdownBlockParser` segments a canonical message into presentation blocks where a single flat `Label` is no longer sufficient.

Current segmentation is:

```text
canonical FormattedText
        |
        v
MarkdownBlockParser
        |
        +-- block_text
        +-- block_code
        +-- block_text
```

`ConversationMessageView` then maps those blocks to framework components:

```text
block_text -> wrapped selectable Label
block_code -> dedicated CodeBlockView
```

`ConversationView::MessageEntry` retains the original source separately from this presentation tree. This is intentional groundwork for re-rendering, Copy Markdown, transport diagnostics, links, persistence, and future richer Markdown widgets.

## Supported subset

The Markdown presentation currently recognizes:

- ATX headings (`#` through `######`) with bold/larger presentation,
- unordered list lines beginning with `* `, `- `, or `+ `,
- ordered list lines beginning with `N. `,
- blockquotes beginning with `>`,
- horizontal rules made from `---`, `***`, or `___`,
- fenced code blocks delimited by triple backticks,
- a first language/info token after an opening code fence,
- inline strong text using `**text**` or `__text__`,
- inline emphasis using `*text*` or `_text_`,
- inline code using backticks,
- backslash escaping only for following Markdown punctuation; literal backslashes before
  ordinary characters remain visible, including Windows paths such as
  `C:\Documents and Settings\...\Received`.

List prefixes remain visible canonical text. This keeps lists readable when copied and works naturally with the composer list model.

Fenced-code delimiters are presentation syntax and are omitted from the dedicated code body.

## Mixed messages

A message is no longer forced to be entirely prose or entirely code. For example:

````text
Hello from Pentium 4

```python
print("MSN Messenger :D")
```

Back to normal :D
````

is presented as:

```text
role header
wrapped text block
CodeBlockView
wrapped text block
```

The role header is separated for block-structured messages so all blocks align beneath the speaker identity.

## Composer code mode

The message toolbar includes an explicit `<code />` toggle plus a native-backed indentation selector (2/4/6/8 spaces). When a draft is sent while code mode is active, `MessageComposer` wraps the canonical body in a fenced Markdown block before creating the `MessageDraft`.

Code mode changes editing semantics:

```text
Enter        -> newline
Shift+Enter  -> newline
Tab          -> configured indentation spaces
Ctrl+Enter   -> send
Ctrl+;       -> toggle code mode
```

See `docs/CODE_COMPOSER.md` for the composer/viewport contract.

Local composer code mode currently emits an unlabelled fence, so its dedicated conversation block displays the generic language title `Code`. Markdown received from a future SaaS backend can immediately use language tags such as `python`, `c`, or `cpp`.

## Interaction with existing rich formatting

Toolbar formatting and Markdown formatting remain complementary. Existing per-character Bold/Italic/Underline/font-size data is retained by canonical `FormattedText`; Markdown semantic presentation is layered above it.

`FormattedText::substring()` preserves per-character formats when the block parser slices canonical text into presentation blocks.

Dedicated code blocks intentionally normalize their visible code body to literal 11pt code presentation so syntax characters are not accidentally displayed with arbitrary prose styling.

## Interaction with emoticons

Classic aliases such as `:)`, `:D`, `:'(`, and `<3` remain canonical text.

Presentation now distinguishes context:

- ordinary text may render those aliases as graphical classic-messenger-inspired emoticons,
- inline code and fenced code carry code semantics,
- the Win32 renderer suppresses emoticon substitution when code semantics are active,
- dedicated `CodeBlockView` bodies therefore keep aliases literal.

For example:

```text
Normal :D          -> graphical grin
`literal :D`       -> literal :D
fenced code :D     -> literal :D
```

## Dedicated fenced code presentation

Fenced code is no longer flattened into gray lines inside the main message label. `CodeBlockView` provides:

- one coherent bordered/background container,
- a header with language label,
- a Copy button,
- selectable Courier New code,
- no soft wrapping,
- independent horizontal overflow scrolling,
- literal emoticon-like tokens.

See `docs/CODE_BLOCKS.md` for the detailed contract and validation checklist.

## Conversation wrapping

Ordinary text blocks use presentation-only soft wrapping through `TextWrapLayout`. Soft wraps do not become canonical newlines. Resizing can therefore reflow prose while copied text preserves its original source.

Code blocks deliberately do not use prose wrapping; long source lines scroll horizontally instead.

## Deliberate current limits

This remains a compact Salix Markdown subset rather than full CommonMark. Follow-up work includes:

- clickable links and URL hit-testing,
- nested-list structure beyond preserved leading spaces,
- tables,
- task-list widgets,
- images,
- full CommonMark delimiter/flanking rules,
- tilde code fences,
- syntax highlighting,
- composer language selection,
- richer quote/list block components.

These features should build on retained canonical source instead of pushing Markdown-specific rules into generic Win32/GDI rendering code.

## Validation checklist

Before marking the current Markdown/block tranche target-validated:

1. Send a numbered list and verify the role appears on its own line with all list items aligned beneath it.
2. Repeat with bullet lists.
3. Send ordinary single-line text and verify compact `You: message` layout remains.
4. Send a multiline plain paragraph and verify role/content block separation.
5. Test `**bold**`, `*italic*`, headings, blockquotes, horizontal rules and inline code.
6. Send prose + fenced code + prose in one message and verify three distinct presentation blocks.
7. Verify language-tagged fences produce the correct code header label.
8. Click the code Copy button and verify only the raw code body reaches Notepad.
9. Mix Markdown with graphical emoticons and verify code contexts keep aliases literal.
10. Verify long prose wraps while long code lines use the code-block horizontal scrollbar.
11. Select/copy rendered prose and code independently.
12. Send enough mixed Markdown messages to exercise conversation scrolling and resizing.
13. On Windows Server 2003, render a Windows path containing multiple backslashes and
    confirm every literal path separator remains visible.
14. MiniXP is not an active acceptance target; revisit compatibility after the Server
    2003 feature set is complete and the MiniXP environment is usable again.
