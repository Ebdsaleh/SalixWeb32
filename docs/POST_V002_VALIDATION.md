# Post-v0.0.2 Target Validation

This document supplements `docs/VALIDATION.md` with target observations for the active composer/conversation work after the tagged `v0.0.2` text-editing baseline.

The post-baseline work remains experimental until each tranche is exercised on the intended legacy targets. A successful test on Windows Server 2003 must not be silently treated as MiniXP validation, and vice versa.

## Target environment

Primary observations in this document come from the real Pentium 4 development target:

```text
CPU:        Pentium 4 class
Memory:     2 GB
OS:         Windows Server 2003 SP2 x86
Compiler:   Visual C++ 7.1 / Visual Studio .NET 2003
```

MiniXP remains the secondary smoke-test environment on the same hardware.

## Graphical emoticons

After the VC7.1 same-basename object-file collision was corrected, the real target successfully rebuilt and displayed the graphical classic-emoticon implementation.

Observed on the target:

- graphical classic-style emoticons render in conversation history,
- the toolbar/emoji presentation renders graphical faces,
- canonical aliases remain the underlying message data.

Detailed interaction coverage remains recorded in `docs/EMOTICON_RENDERING.md`.

## Multiline and list editing

The target has displayed and exercised multiline/list composition including continued numbered/bulleted lines. Subsequent list behavior introduced context-sensitive Enter semantics and `Ctrl+Enter` forced send.

Observed examples include:

```text
* item
* next item
```

and:

```text
1. one
2. two
3. three
4.
```

The list state participates in the richer composer rather than being rendered as a one-off static demonstration.

## Markdown presentation

The Windows Server 2003 target successfully rebuilt and displayed Markdown-oriented conversation presentation.

Observed presentation included:

- role headers separated from block-style content,
- numbered list alignment beneath `You:`,
- headings/emphasis rendered through formatted text,
- normal text and graphical emoticons continuing to render.

## Code composer mode

The target successfully displayed the `<code />` toolbar control, indentation selector, multiline code input, and code submission path.

Observed behavior/presentation includes:

- code-mode content reaches conversation history,
- code text is visibly distinguished from ordinary text,
- Courier-style code presentation is active,
- source indentation survives submission,
- code aliases such as `<3` remain literal where code semantics are active,
- ordinary text can still render the graphical heart/emoticon presentation.

The target screenshots therefore confirm that normal and code semantic paths are visibly distinct.

## Legacy mouse-wheel compatibility

The native conversation scrolling tranche initially failed to compile because the legacy SDK headers did not expose `WM_MOUSEWHEEL`.

The compatibility fallback:

```cpp
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
```

was added without globally raising the Win32 target version. The user rebuilt after the fix and reported that it works.

This confirms the compatibility fix on the primary Server 2003/VC7.1 target.

## Wrapped rich conversation viewport

The wrapped/pixel-scrolled conversation build has successfully compiled and run on the Windows Server 2003 target. Screenshots confirm the richer normal-vs-code presentation operating inside the scrollable conversation surface.

The following full checklist is still pending before calling the tranche completely validated:

- very long ordinary paragraph soft wrapping,
- copy of soft-wrapped prose proving no new canonical line breaks were inserted,
- repeated narrow/wide resize reflow,
- one response taller than the whole viewport,
- selection across several soft-wrapped lines,
- exhaustive native scrollbar/thumb/wheel behavior with huge messages,
- equivalent MiniXP smoke coverage.

See `docs/RICH_CONVERSATION_VIEWPORT.md`.

## Block-composed code presentation

The block-composed presenter replaces flattened fenced-code presentation with a message tree:

```text
ConversationMessageView
    +-- role header
    +-- wrapped text Label
    +-- CodeBlockView
    +-- wrapped text Label
```

The latest real-target video confirms that the dedicated code block is now being created and presented rather than falling back to ordinary conversation text. The code body is visibly separated from surrounding prose and the `Copy` affordance is present. This is a major target-validation milestone for the block-composition path.

The remaining validation work includes:

- language-token display from fenced Markdown,
- Copy-to-Notepad verification for the whole code block,
- partial code selection with Ctrl+C,
- literal `:)`, `:D`, and `<3` inside code while prose aliases remain graphical,
- long unwrapped code lines and the inner horizontal scrollbar,
- outer vertical scrolling while a code block is partially visible,
- repeated resize/reflow,
- multiple code blocks in one message,
- equivalent MiniXP smoke coverage.

See `docs/CODE_BLOCKS.md` for the detailed contract.

## Mouse-move redraw regression

The same target video exposed a separate native-control rendering regression: moving the mouse around the application caused visible redraw/flicker behaviour in the composer, making the UI appear unstable even though the code-block content itself was working.

Investigation found that `MessageInputStrip::handle_event()` recalculated its text extents and called `update_scrollbars()` after **every** routed event, including an idle `WM_MOUSEMOVE` that the underlying `TextInput` did not handle. `update_scrollbars()` eventually synchronizes the native Win32 scrollbar peers, so ordinary pointer motion could repeatedly drive `MoveWindow`/scrollbar synchronization on the legacy target.

The corrective change now:

- returns immediately when the text input did not handle the routed event,
- recalculates/synchronizes composer scrollbars only after a meaningful handled input event,
- keeps caret visibility updates on handled input only,
- makes the fallback content-width estimator respect `TextFormat::code_block`, so code aliases are measured as literal text instead of graphical emoticons.

This fix is **pending target validation**. The key regression test is simple: leave the composer idle and move the mouse rapidly across the window. Native scrollbars and the composer surface must remain visually stable. Then repeat normal selection/dragging, code-mode typing, list entry, and scrollbar interaction to confirm no legitimate refresh path was lost.
