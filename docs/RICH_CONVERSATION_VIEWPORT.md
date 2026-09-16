# Rich Conversation Viewport

This document records the conversation-layout work that follows native conversation scrolling. The wrapped/pixel-scrolled viewport has built and displayed successfully on the real Windows Server 2003 Pentium 4 target; the newer dedicated block-composition/code-container tranche remains pending target validation.

## Goal

Conversation history must behave like a document surface rather than a stack of fixed message rows. Long prose should wrap to the available width, one response may be taller than the viewport, scrolling must move through that response, and structured Markdown content must be able to become multiple presentation components without rewriting canonical message data.

## Canonical text and presentation

Soft wrapping is presentation-only. `TextWrapLayout` calculates visual line ranges from canonical text and per-character `TextFormat` data. It does not insert `\n` characters into the message.

Therefore:

- resizing can reflow prose without rewriting message content,
- copying a paragraph returns its original text rather than viewport-created line breaks,
- Markdown source remains separate from rendered presentation,
- selection ranges continue to refer to canonical source indices.

`Label` has an explicit `word_wrap` presentation property. Wrapped label hit-testing uses the same `TextWrapLayout` model used by rendering so pointer selection maps back to canonical source positions.

## Pixel-scrolled conversation history

`ConversationView` stores a vertical pixel offset rather than an index identifying the first visible message. Every conversation entry retains its complete rendered height and position in one continuous content surface.

The native Win32 conversation `SCROLLBAR` value therefore represents a pixel offset. This permits one message whose rendered height is greater than the viewport to be traversed progressively rather than skipped as one indivisible entry.

Mouse-wheel input scrolls the same pixel viewport. Message text, Markdown source, formatting, selection and clipboard data are not mutated by scrolling.

## Clipping

`ComponentRenderer` exposes a backend-neutral clip-stack contract:

```text
push_clip_rect(x, y, width, height)
render children
pop_clip_rect()
```

The Win32 renderer implements this with `SaveDC`, `IntersectClipRect`, and `RestoreDC`. `ConversationView` clips all historical-message drawing to its viewport while allowing partially visible entries to retain their true full bounds for hit-testing and layout.

## Layout metrics

Accurate wrapping depends on the platform text metrics actually used to render each formatted run. The `View::layout` contract can receive an optional backend-neutral `TextMetrics*`.

`Win32ApplicationHost` supplies `Win32TextMetrics` while laying out the application, allowing the conversation surface to calculate wrapped heights using the same font widths used for drawing and pointer hit-testing.

A conservative fallback remains available if metrics are temporarily unavailable.

## Block-composed messages

The conversation presenter is no longer limited to one `Label` per message.

The current direction is:

```text
canonical FormattedText message
        |
        v
MarkdownBlockParser
        |
        +-- text block
        +-- fenced code block
        +-- text block
        |
        v
ConversationMessageView
        |
        +-- role header
        +-- wrapped selectable Label
        +-- CodeBlockView
        +-- wrapped selectable Label
```

`ConversationView::MessageEntry` still retains the complete canonical source separately from those presentation components.

Ordinary short single-line messages preserve the compact form:

```text
You: Hello from Pentium 4 :D
```

Messages containing block structure use a separate role header so lists, code blocks, and following prose align beneath the speaker identity.

See `docs/CODE_BLOCKS.md` for the dedicated code-block contract.

## Markdown code semantics

`TextFormat` carries lightweight semantic code state:

```text
code_none
code_inline
code_block
```

Inline backtick spans remain formatted text and use `code_inline`. Fenced code is now segmented at the block level for conversation presentation and becomes a dedicated `CodeBlockView`; the code body itself still uses `code_block` formatting.

On Win32:

- normal text uses Tahoma,
- code uses Courier New,
- inline code receives a light background,
- dedicated code blocks receive one coherent panel/body background,
- graphical emoticon substitution is disabled inside code so source such as `:)` remains literal.

The composer code mode continues to submit fenced Markdown, so its sent output follows the same generic Markdown block path as a future SaaS response.

## Wrapping behavior

The prose wrap implementation is compact and deterministic:

- explicit newlines remain hard line boundaries,
- soft lines prefer the most recent whitespace breakpoint that fits,
- a token longer than the available width is split rather than escaping the viewport,
- graphical emoticon aliases are treated as one visual token outside code,
- code blocks deliberately do not soft-wrap and instead use horizontal overflow scrolling.

This is not yet a full Unicode line-breaking implementation. Unicode/text-shaping work remains a later web/text-platform concern.

## Current limits

The current conversation viewport still does not provide:

- syntax highlighting,
- a full CommonMark parser,
- clickable links/URL hit-testing,
- tables/task-list widgets,
- images embedded in Markdown,
- Unicode line-breaking/shaping rules,
- a finalized nested-native-HWND clipping policy for code-block scrollbars.

The dedicated code block now provides language chrome, a Copy button, selectable literal code, and horizontal overflow through the framework scrollbar fallback. See `docs/CODE_BLOCKS.md` for why nested code scrollbars are not yet native Win32 child controls.

## Validation status

The wrapped rich viewport and normal-vs-code presentation have been observed building and running on the real Windows Server 2003 SP2 Pentium 4 target. The target screenshots confirm distinct normal/code presentation and literal code aliases versus graphical emoticons in ordinary text.

Full validation of every wrap, selection, resize and giant-single-message case is still pending. The block-composed code-container work is newer and requires a fresh VC7.1 rebuild.

## Validation checklist

Before marking the combined rich conversation viewport fully validated:

1. Build and link with Visual C++ 7.1.
2. Send a long ordinary paragraph and verify it wraps to the conversation width.
3. Copy the wrapped paragraph to Notepad and verify soft wraps did not become inserted newlines.
4. Resize the window narrower and wider and verify the message reflows and row heights update.
5. Send one response tall enough to exceed the whole conversation viewport and verify the native scrollbar can move through the middle of that single response.
6. Verify mouse-wheel movement is pixel-based rather than jumping whole messages.
7. Select/copy text across several soft-wrapped visual lines and verify canonical selection content remains correct.
8. Verify inline code uses literal code presentation while ordinary emoticons outside code remain graphical.
9. Verify fenced code becomes a dedicated block container with language header, Copy button, and non-wrapped code body.
10. Exercise headings, lists, blockquotes, mixed font sizes and attachments while scrolling.
11. Repeatedly resize the window and verify scrollbar range/visibility remain coherent.
12. Repeat the build/runtime tests under Windows Server 2003 SP2 and MiniXP.
