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

## Composer/native-scrollbar redraw regression

The first target video exposed a native-control rendering regression: moving the mouse around the application caused visible redraw/flicker behaviour in the composer even when no edit was taking place.

The first correction stopped `MessageInputStrip::handle_event()` from recalculating its text extents and calling `update_scrollbars()` for an **unhandled** idle `WM_MOUSEMOVE`. The user rebuilt that change on the primary target and confirmed that the idle-mouse "ghost" disappeared.

A second target observation then exposed a narrower residual problem: clicking in the composer and drag-selecting text could still make the native scrollbars visibly refresh or appear temporarily active even though the document dimensions had not changed.

The reason is architectural rather than text-specific. A handled selection event can legitimately flow through caret/viewport maintenance, and `apply_viewport_state()` may call `sync_scroll_bar()`. The old Win32 peer synchronization performed all of these native operations on every sync request regardless of whether anything changed:

```text
MoveWindow(..., TRUE)
ShowWindow(...)
SetScrollInfo(..., TRUE)
EnableWindow(...)
```

That is effectively a small "refresh all" for the scrollbar HWND. On the legacy target this is unnecessarily visible.

The new synchronization contract is delta-based. Each Win32 scrollbar peer caches its last native state and only performs an operation when the corresponding state actually changes:

- geometry -> `MoveWindow` only when x/y/width/height changes,
- visibility -> `ShowWindow` only when visibility changes,
- range/page/value -> `SetScrollInfo` only when scroll information changes,
- enabled state -> `EnableWindow` only when overflow availability changes.

Selection/focus changes therefore remain framework events, but they should no longer churn the native scrollbar HWND when geometry and overflow state are unchanged. Actual text growth, resize, scrolling, or range changes still synchronize normally.

The fallback content-width estimator also respects `TextFormat::code_block`, so code aliases are measured as literal text instead of graphical emoticons.

The delta-synchronization refinement is **pending explicit target validation**. Regression tests:

1. Leave the composer empty or below overflow thresholds and click repeatedly inside it; scrollbars must remain visually stable/inactive.
2. Drag-select short text; the selection should update without native scrollbar flashing.
3. Type enough rows to exceed the vertical viewport; only then should the vertical scrollbar become enabled.
4. Type a sufficiently long unwrapped line; only then should the horizontal scrollbar become enabled.
5. Reduce the document below each threshold and confirm the corresponding scrollbar disables again.
6. Exercise real scrollbar arrows/thumbs after overflow and confirm value updates still redraw correctly.
7. Repeat under MiniXP after the Server 2003 result is clean.

## Pending code-language and syntax tranche

The next code-presentation tranche is now implemented in source but has not yet been rebuilt on the Pentium 4.

It adds:

- a shared `CodeLanguageRegistry`,
- a native-backed composer language selector next to `<code />`,
- canonical Markdown language tokens generated by local code composition,
- language labels driven by the same registry in `CodeBlockView`,
- `TextFormat::SyntaxStyle`,
- a lightweight backend-neutral `CodeSyntaxHighlighter`,
- token families for C-like languages, Python, JSON, Shell, Lua, HTML/XML, and CSS,
- source-preserving keyword/string/comment/number/preprocessor/literal/tag syntax metadata.

The language selector and indentation selector are intentionally disabled while code mode is off and enabled while code mode is active.

Target validation must establish:

1. Visual C++ 7.1 compiles and links the new registry/highlighter translation units.
2. The language selector is a real native Win32 combo box on Server 2003.
3. `Python`, `C++`, `JavaScript`, `JSON`, and other selections survive Send as canonical Markdown fence tokens.
4. The resulting conversation header reports the correct language.
5. Syntax emphasis remains aligned with source selection/hit-testing and does not alter copied text.
6. Code aliases stay literal while prose aliases remain graphical.
7. The incremental native-scrollbar behavior remains stable with the wider toolbar and new combo box.
8. MiniXP receives a smoke pass only after Server 2003 succeeds.

See `docs/CODE_COMPOSER.md`, `docs/CODE_BLOCKS.md`, and `docs/SYNTAX_HIGHLIGHTING.md`.

## Pending composer mouse-wheel and vertical-caret tranche

The composer input now has an explicit wheel-routing path and a persistent vertical navigation goal. This work is implemented in source and remains pending target validation.

Mouse-wheel behavior is intentionally viewport-local:

```text
pointer over composer text viewport
    Wheel          -> vertical scroll
    Shift + Wheel  -> horizontal scroll
```

One notch advances three configured scrollbar line steps. The wheel path changes only the semantic scrollbar value and applies the existing `TextViewportState`; it does not rebuild text layout merely because the mouse wheel moved. If the requested axis cannot move, the event is left unconsumed so a future enclosing scroll container can receive it.

The vertical caret fix addresses a separate editing defect observed on the real target. The previous Up/Down implementation recomputed `column = caret - line_start` after every move. A short intermediate line could therefore collapse the remembered column, causing a later Up movement into a longer line to land several positions to the left. It also ignored the fact that normal formatted text uses proportional widths.

The new behavior captures a preferred visual X on the first Up/Down using backend-neutral `TextMetrics`. Repeated vertical moves keep that goal even if a short line temporarily clamps the caret. Win32 supplies `Win32TextMetrics` only for Up/Down key-down events, avoiding unnecessary measurement setup for unrelated keys. Backends without metrics retain a persistent logical-column fallback.

Target validation should verify:

1. Vertical wheel scrolling works while the pointer is inside the composer text viewport and the vertical scrollbar has overflow.
2. `Shift+Wheel` scrolls long unwrapped content horizontally.
3. Wheel movement changes the viewport/thumb but does not move the caret or alter draft text.
4. Wheel input at a boundary/no-overflow state does not cause scrollbar flashing or unnecessary native-control churn.
5. Up/Down between sufficiently long lines preserves the caret's visual X.
6. A long-line -> short-line -> long-line sequence temporarily clamps on the short line but restores the original preferred X on the next long line.
7. Mixed proportional formatting and Courier code mode behave sensibly.
8. `Shift+Up`/`Shift+Down` extends selection using the same preferred X.
9. Left/Right, Home/End, pointer repositioning, editing, and focus changes begin the next vertical sequence from the new caret location.
10. Server 2003 is validated first, followed by MiniXP smoke coverage.

See `docs/CODE_COMPOSER.md` for the complete interaction contract.

## Pending native context-menu and conversation-selection tranche

The first native context-menu pass established native Win32 popup presentation and message-local selection. Target use then exposed two desktop-behavior ambiguities:

1. a generic `Select All` menu item did not identify whether it meant one System/User/Remote message or the whole conversation,
2. normal drag selection stopped at the boundary of the message row where the drag began.

The current refinement moves read-only selection coordination up to `ConversationView` and makes menu scope explicit.

When right-clicking a message row, the menu now offers a role-specific command plus a conversation-wide command, for example:

```text
Copy
-----------------------
Select All in System Message
Select All Conversation
```

The role label changes to `User` or `Remote` as appropriate. Right-clicking conversation whitespace omits the role-specific command and exposes only `Select All Conversation` for selection scope.

A normal drag may now continue across message rows and across the internal role/prose/code/prose surfaces of those rows. Intermediate messages are fully selected. Backward dragging is symmetric. Dragging above/below the viewport while the mouse is captured performs line-step autoscroll so selection can extend through history that was not initially visible.

Conversation-wide Copy collects selected message presentations in document order with message/block boundaries represented as line breaks. The code-block `Copy` button retains its existing narrower meaning of copying only raw code.

Target validation should verify:

1. Visual C++ 7.1 compiles and links `ConversationMessageSelection.cpp` without warnings/errors.
2. Message context menus identify `System`, `User`, or `Remote` scope explicitly.
3. Whitespace context menus do not imply a hidden message scope.
4. `Select All Conversation` selects every message, including scrollable history outside the current viewport.
5. Drag selection crosses System -> User -> Remote message boundaries in both directions.
6. A drag can cross prose -> code -> prose and then continue into another message.
7. Dragging beyond the top/bottom viewport edge autoscrolls and extends the selection.
8. Multi-message Copy into Notepad preserves presentation order and useful line breaks.
9. Ctrl+C and Ctrl+A operate on the conversation after the conversation becomes the active selection context.
10. Existing composer context-menu editing remains unchanged.
11. The code-block `Copy` button still copies raw code only.
12. Native scrollbars do not regress while drag-autoscroll or popup menus are active.
13. Validate first on Windows Server 2003 SP2 and then repeat the smoke pass under MiniXP.

See `docs/CONTEXT_MENUS.md` for the complete interaction contract and checklist.
