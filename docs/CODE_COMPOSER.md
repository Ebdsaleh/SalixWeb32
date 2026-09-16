# Code Composer Mode and Input Viewport

This document records the post-Markdown composer tranche that adds an explicit code-entry mode, configurable language/indentation metadata, scrolling, and richer multiline navigation to the message-composition input surface. Windows Server 2003 target smoke coverage exists for the earlier code-mode/viewport implementation; the newest language-selector, mouse-wheel, and preferred-column caret additions remain pending VC7.1/Server 2003/MiniXP validation.

## Toolbar controls

Three code-oriented controls sit immediately after the emoticon button:

```text
[:)] [<code />] [Python v] [4 v]
```

`<code />` is the existing toggle button. The language and indentation selectors are backend-neutral `ComboBox` components using native Win32 COMBOBOX peers on the Win32 backend.

The code-language selector is disabled while code mode is off and currently exposes:

```text
Code
C
C++
C#
Java
Python
JavaScript
TypeScript
JSON
Shell
Rust
Lua
HTML
CSS
XML
Plain text
```

`Code` means an unlabelled Markdown fence. The registry stores canonical transport tokens (`cpp`, `python`, `javascript`, `bash`, and so on) separately from the display names so the conversation renderer and future transports do not have to infer semantics from UI text.

The indentation selector remains:

```text
2, 4, 6, 8
```

with 4 spaces as the default. Both language and indentation controls become enabled when code mode is active.

## Code-mode shortcut

`Ctrl+;` toggles code-entry mode. The shortcut is routed from the backend-neutral semicolon key event rather than depending only on a generated character message. The follow-up Ctrl+semicolon character is consumed so the shortcut cannot toggle twice or insert an unwanted semicolon.

The mouse and keyboard paths drive the same toolbar state.

Code state is recorded per text range by the composer formatting model, so code typed while the toggle is active can survive toggling back to ordinary prose before Send. The selected language is currently composer-wide metadata: every code range emitted by one draft uses the currently selected language token. Per-block language selection is a future richer-editor refinement.

## Enter behavior

Outside code mode, the existing messenger/list rules remain:

```text
ordinary line: Enter -> Send
list line:     Enter -> continue list
Shift+Enter          -> plain newline
Ctrl+Enter           -> Send
```

While code mode is checked:

```text
Enter       -> newline
Shift+Enter -> newline
Ctrl+Enter  -> Send
```

This deliberately gives code editing the same "do not accidentally send on Enter" protection as active list entry while retaining a direct keyboard Send command.

The Send button remains available to pointer users.

## Tab behavior

While code mode is active, `Tab` is consumed by the composer and inserts spaces at the caret instead of participating in focus traversal. The number of inserted spaces comes from the indentation combo box.

The stored draft contains ordinary space characters rather than literal tab characters. This keeps layout deterministic on legacy renderers and across future transports.

Outside code mode, Tab remains available for future normal focus-navigation behavior.

## Markdown transport/presentation

Code ranges are serialized as canonical fenced Markdown before `MessageDraft` submission. The selected language becomes the fence token:

````text
```python
print("Hello from Pentium 4")
```
````

If `Code` is selected, the fence remains unlabelled:

````text
```
code here
```
````

The conversation block parser therefore receives ordinary portable Markdown rather than a private code-message object. The parser extracts the fence token and passes it to `CodeBlockView`, which uses `CodeLanguageRegistry` for a stable display label and `CodeSyntaxHighlighter` for lightweight language-aware token semantics.

This keeps composer output directly useful to future ChatGPT/SaaS transports and Markdown persistence.

## Composition scrollbars

The message-entry surface reserves classic horizontal and vertical scrollbars around the editable viewport. This is necessary because the multiline composer intentionally does not word-wrap long logical lines yet and can also contain more lines than fit vertically.

`framework/ScrollBar` owns the backend-neutral semantic range/value/page state and retains a framework-rendered fallback for non-native backends.

On Win32, `Win32NativeControlHost` attaches real child-window `SCROLLBAR` peers using `SBS_HORZ` and `SBS_VERT`. `Win32ApplicationHost` routes `WM_HSCROLL` and `WM_VSCROLL` back into the semantic `ScrollBar`, so Server 2003 and MiniXP receive their normal operating-system scrollbar arrows, tracks and thumbs rather than a painted approximation.

The scroll model supports:

- horizontal and vertical orientations,
- line-step arrow movement,
- page-step track movement,
- draggable/native thumbs,
- bounded value/range/page state,
- value-change callbacks.

The horizontal bar exposes long code/text lines; the vertical bar exposes drafts taller than the visible editor.

The text model itself remains unchanged. `TextViewportState` stores presentation-only scroll offsets for the custom-rendered `TextInput`, and the Win32 scroll-aware input painter clips rendering to the editor viewport before applying those offsets.

Mouse hit-testing is translated through the same viewport offsets before reaching the underlying `TextInput`, so selection/caret behavior continues to address canonical source positions.

The composer also keeps the caret visible after editing/navigation by adjusting the scroll values when the caret moves outside the current viewport.

Native scrollbar peer synchronization is delta-based: selection/focus changes do not call `MoveWindow`, `SetScrollInfo`, `ShowWindow`, or `EnableWindow` unless the associated native state actually changed. This prevents click/drag-selection activity from visually activating otherwise inactive scrollbars on the legacy target.

## Mouse-wheel scrolling

`MessageInputStrip` now owns mouse-wheel behavior for the editable viewport instead of relying only on the native scrollbar arrows/thumbs.

When the pointer is over the actual text-input viewport:

```text
Wheel          -> vertical viewport scroll
Shift + Wheel  -> horizontal viewport scroll
```

One wheel notch advances three configured scrollbar line steps. The event changes only the semantic scrollbar value and applies the corresponding `TextViewportState`; it does not mutate the draft, move the caret, or recalculate the document extent merely because the wheel moved.

If the selected axis cannot move because there is no overflow or the viewport is already at the requested boundary, the composer does not consume that wheel event. This preserves a path for an enclosing scrollable surface to handle it later rather than trapping the wheel at a dead end.

## Vertical caret navigation

Multiline Up/Down navigation now keeps a persistent preferred horizontal target for a consecutive vertical-navigation sequence.

The original implementation recomputed a character column after every move. That meant moving through a shorter line permanently collapsed the remembered column; going back to a longer line could leave the caret several characters to the left of where the sequence began. It also treated character count as visual position even when Tahoma, bold/italic text, different font sizes, graphical emoticons, or Courier code made glyph widths differ.

The new contract is:

1. The first Up/Down captures the current logical column and, when `TextMetrics` is available, the rendered pixel X of the caret.
2. Each target line maps that preferred pixel X back to the nearest legal character boundary.
3. A shorter target line may temporarily clamp the caret to its end, but the preferred X remains unchanged.
4. Moving again to a longer line restores the original visual column as closely as the target line permits.
5. `Shift+Up`/`Shift+Down` use the same preferred-X behavior while extending selection.
6. Horizontal navigation, Home/End, pointer repositioning, text edits, formatting changes, undo/redo restoration, focus changes, and explicit cursor changes reset the vertical-navigation goal.

The Win32 host supplies a `Win32TextMetrics` instance specifically for Up/Down key-down events, so the backend-neutral `TextInput` can request real formatted measurements without importing Win32 APIs or paying the measurement setup cost for unrelated key events.

If a backend does not supply text metrics, `TextInput` falls back to persistent logical-column behavior; this still fixes the short-line collapse even without pixel measurement.

## Validation checklist

Before marking the newest composer tranche target-validated:

1. Verify the toolbar shows `<code />`, the native language selector, and the native 2/4/6/8 indentation selector after the emoticon control.
2. Verify language/indent controls are disabled while code mode is off and enabled when it is on.
3. Toggle code mode with the mouse and with `Ctrl+;`; verify the shortcut toggles once and does not insert `;`.
4. Select Python, C++, JavaScript, JSON, and at least one other language; send code and verify the conversation code-block header reports the chosen language.
5. Verify the canonical Markdown generated for code contains the corresponding language fence token.
6. In code mode, verify Enter and Shift+Enter both create new lines and do not send; verify Ctrl+Enter sends.
7. Verify Tab inserts exactly 2/4/6/8 spaces according to the combo setting.
8. Verify code typed while code mode is active remains a code range if the toggle is switched off before ordinary prose is appended.
9. Verify the composition area uses native Server 2003/MiniXP horizontal and vertical scrollbar controls without click/selection redraw churn.
10. Create vertical overflow, hover over the text viewport, and verify the mouse wheel moves the vertical viewport and native thumb without moving the caret.
11. Create a long unwrapped line and verify `Shift+Wheel` moves the horizontal viewport.
12. At a scroll boundary or with no overflow, verify wheel input stops cleanly without spurious native-control redraws.
13. Test Up/Down between equal-length normal-text lines and verify the caret remains at the same visual X.
14. Test a long line -> short line -> long line sequence; the short line may clamp to its end, but the following long line must return to the original preferred visual column.
15. Repeat vertical navigation with mixed formatting and in Courier code mode, and verify `Shift+Up`/`Shift+Down` extends selection along the same preferred X.
16. Verify Left/Right, Home/End, mouse clicks, and subsequent Up/Down start a fresh vertical goal from the new caret position.
17. Verify caret placement and drag selection remain correct after scrolling on both axes.
18. Verify automatic caret-follow scrolling keeps keyboard editing visible near the right/bottom edges.
19. Verify Undo/Redo, clipboard operations, formatting, list mode, attachments, and emoticon insertion do not regress.
20. Repeat smoke coverage under Windows Server 2003 SP2 and MiniXP.
