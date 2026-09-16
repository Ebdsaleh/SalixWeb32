# Rich Conversation Viewport

This document records the conversation-layout tranche that follows native conversation scrolling. The work remains pending target validation on Visual C++ 7.1 / Windows Server 2003 SP2 and MiniXP.

## Goal

Conversation history must behave like a real SaaS/chat document surface rather than a stack of fixed message rows. Long prose should wrap to the available width, one response may be taller than the viewport, and scrolling must move through that response without modifying its canonical text.

## Canonical text and presentation

Soft wrapping is presentation-only. `TextWrapLayout` calculates visual line ranges from the existing canonical text and per-character `TextFormat` data. It does not insert `\n` characters into the message.

Therefore:

- resizing can reflow text without rewriting message content,
- copying a paragraph returns its original text rather than viewport-created line breaks,
- Markdown source remains separate from rendered presentation,
- selection ranges continue to refer to canonical source indices.

`Label` now has an explicit `word_wrap` presentation property. Wrapped label hit-testing uses the same `TextWrapLayout` model used by rendering so pointer selection maps back to canonical source positions.

## Pixel-scrolled conversation history

`ConversationView` now stores a vertical pixel offset instead of an index identifying the first visible message. Every message retains its complete row height and position in one continuous content surface.

The native Win32 `SCROLLBAR` value therefore represents a pixel offset. This permits a single message whose rendered height is greater than the conversation viewport to be traversed progressively rather than skipped as one indivisible entry.

Mouse-wheel input scrolls the same pixel viewport. Message text, Markdown source, formatting, selection and clipboard data are not mutated by scrolling.

## Clipping

`ComponentRenderer` now exposes a backend-neutral clip-stack contract:

```text
push_clip_rect(x, y, width, height)
render children
pop_clip_rect()
```

The Win32 renderer implements this with `SaveDC`, `IntersectClipRect`, and `RestoreDC`. `ConversationView` clips all historical-message drawing to its viewport while allowing partly visible labels to keep their true full bounds for hit-testing and layout.

## Layout metrics

Accurate wrapping depends on the platform text metrics actually used to render each formatted run. The `View::layout` contract can now receive an optional backend-neutral `TextMetrics*`.

`Win32ApplicationHost` supplies `Win32TextMetrics` while laying out the application, allowing `ConversationView` to compute wrapped row heights using the same font widths used for drawing and pointer hit-testing.

A conservative fallback remains available if metrics are temporarily unavailable.

## Markdown code semantics

`TextFormat` now carries a lightweight semantic code style:

```text
code_none
code_inline
code_block
```

`MarkdownFormatter` marks inline backtick spans as `code_inline` and fenced code content as `code_block`. The canonical Markdown punctuation remains source data; the semantic state exists only in formatted presentation output.

On Win32:

- normal text uses Tahoma,
- code uses Courier New,
- inline code receives a light background,
- fenced code lines receive a code-block background,
- graphical emoticon substitution is disabled inside code so source such as `:)` remains literal code text.

The composer code mode continues to submit fenced Markdown, so its sent output follows the same generic Markdown-code presentation path as a future SaaS response.

## Wrapping behavior

The first wrap implementation is deliberately compact and deterministic:

- explicit newlines always remain hard line boundaries,
- soft lines prefer the most recent whitespace breakpoint that fits,
- a token longer than the available width is split rather than escaping the viewport,
- graphical emoticon aliases are treated as one visual token outside code,
- code text is measured as literal monospace characters.

This is not yet a full Unicode line-breaking implementation. Unicode/text-shaping work remains a later web/text-platform concern.

## Current limits

This tranche intentionally does not yet provide:

- syntax highlighting,
- code-block Copy buttons,
- per-code-block horizontal scrolling,
- language labels on fenced code blocks,
- a full CommonMark parser,
- Unicode line-breaking/shaping rules.

Those features can build on the new wrapped/pixel-scrolled viewport without changing canonical message storage.

## Validation checklist

Before marking this tranche validated:

1. Build and link with Visual C++ 7.1.
2. Send a long ordinary paragraph and verify it wraps to the conversation width.
3. Copy the wrapped paragraph to Notepad and verify soft wraps did not become inserted newlines.
4. Resize the window narrower and wider and verify the message reflows and row heights update.
5. Send one response tall enough to exceed the whole conversation viewport and verify the native scrollbar can move through the middle of that single response.
6. Verify mouse-wheel movement is smooth and pixel-based rather than jumping whole messages.
7. Select/copy text across several soft-wrapped visual lines and verify canonical selection content remains correct.
8. Send Markdown containing inline backticks and fenced code; verify Courier New/background presentation.
9. Put `:)`, `:D`, or `<3` inside a code block and verify it remains literal text rather than becoming a graphical emoticon.
10. Verify graphical emoticons outside code continue to render normally.
11. Exercise headings, lists, blockquotes, mixed font sizes and attachments while scrolling.
12. Repeatedly resize the window and verify native scrollbar range/visibility remain coherent.
13. Repeat the build/runtime tests under Windows Server 2003 SP2 and MiniXP.
