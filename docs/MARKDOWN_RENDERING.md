# Markdown Conversation Presentation

This document records the first native Markdown presentation layer for SalixWeb32 conversation history. Target validation on Visual C++ 7.1 / Windows Server 2003 SP2 / MiniXP is pending.

## Goal

Conversation messages keep their canonical source text while `ConversationView` derives a presentation-only `FormattedText` representation for display. This gives local and future remote/SaaS messages a richer presentation without replacing the portable message payload.

The immediate motivation is block alignment. A canonical list such as:

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

The parser lives in `framework/MarkdownFormatter`. It is backend-neutral and converts canonical message text (including existing per-character `TextFormat`) into presentation `FormattedText`.

`ConversationView::MessageEntry` retains the original `FormattedText` source separately from the rendered `Label`. This is intentional groundwork for future features such as re-rendering, Copy Markdown, transport diagnostics, links, and richer block widgets.

The Win32 renderer remains a generic formatted-text renderer. Markdown syntax is interpreted above the platform rendering layer rather than embedding Markdown rules into GDI code.

## Initial supported subset

The first formatter recognizes:

- ATX headings (`#` through `######`) with bold / larger presentation,
- unordered list lines beginning with `* `, `- `, or `+ `,
- ordered list lines beginning with `N. `,
- blockquotes beginning with `>`,
- horizontal rules made from `---`, `***`, or `___`,
- fenced code blocks delimited by triple backticks,
- inline strong text using `**text**` or `__text__`,
- inline emphasis using `*text*` or `_text_`,
- inline code using backticks,
- backslash escaping for a following Markdown punctuation character.

List prefixes remain visible canonical text. This keeps the list readable when selected/copied and works naturally with the existing composer list model.

Fenced-code delimiters are presentation syntax and are omitted from the displayed block. Code contents are not recursively Markdown-formatted.

## Interaction with existing rich formatting

Toolbar formatting and Markdown formatting are complementary. Existing per-character Bold/Italic/Underline/font-size data is used as the source format, then Markdown semantics are layered over it for presentation.

For example, a locally formatted message can still contain Markdown list structure or inline emphasis without flattening the toolbar formatting first.

## Interaction with emoticons

Classic aliases such as `:)`, `:D`, `:'(`, and `<3` remain canonical text after Markdown presentation and therefore continue through the existing graphical-emoticon rendering path.

A future code-span semantic flag should suppress emoticon replacement inside fenced/inline code. The first Markdown tranche does not yet add that renderer-level semantic flag, so an emoticon-looking token inside code can still be interpreted by the graphical-emoticon painter.

## Deliberate first-tranche limits

This is a compact Salix Markdown subset, not a full CommonMark implementation yet. The following remain follow-up work:

- automatic word wrapping and viewport scrolling,
- monospace font-family support for code spans/blocks,
- dedicated code-block background/chrome and Copy button,
- clickable links and URL hit-testing,
- nested-list indentation metadata beyond preserved leading spaces,
- tables,
- task-list checkboxes,
- images,
- full CommonMark delimiter/flanking rules,
- syntax highlighting.

These features should build on the retained canonical source rather than forcing Markdown syntax into the generic text renderer.

## Validation checklist

Before marking this tranche target-validated:

1. Send a numbered list and verify `You:` appears on its own role line with all list items aligned beneath it.
2. Repeat with bullet lists.
3. Send ordinary single-line text and verify the compact `You: message` layout remains.
4. Send a multiline plain paragraph and verify role/content block separation.
5. Test `**bold**`, `*italic*`, headings, blockquotes, horizontal rules, inline code, and fenced code.
6. Mix Markdown with graphical emoticons and existing toolbar formatting.
7. Select/copy rendered Markdown text and verify the selectable read-only conversation behavior remains stable.
8. Send enough Markdown messages to exercise conversation scrolling and resizing.
9. Repeat the validation under Windows Server 2003 SP2 and MiniXP.
