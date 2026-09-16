# Code Composer Mode and Input Viewport

This document records the post-Markdown composer tranche that adds an explicit code-entry mode, configurable indentation, and scrollbars to the message-composition input surface. Visual C++ 7.1 / Windows Server 2003 SP2 / MiniXP target validation is pending.

## Toolbar controls

Two controls sit immediately after the emoticon button:

```text
[:)] [<code />] [4 v]
```

The second control is a real toggle button. The final control is the existing backend-neutral `ComboBox` using a native Win32 COMBOBOX peer on the Win32 backend. Its allowed indentation widths are:

```text
2, 4, 6, 8
```

The default is 4 spaces.

## Code-mode shortcut

`Ctrl+;` toggles code-entry mode. The shortcut is routed from the backend-neutral semicolon key event rather than depending only on a generated character message. The follow-up Ctrl+semicolon character is consumed so the shortcut cannot toggle twice or insert an unwanted semicolon.

The mouse and keyboard paths drive the same toolbar state.

Code mode is message-wide editor state in this tranche. It changes keyboard behavior without replacing the canonical text model.

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

When a code-mode draft is submitted, `MessageComposer` wraps its body in a canonical fenced Markdown code block before creating the `MessageDraft`:

````text
```
code here
```
````

The conversation Markdown presenter therefore receives normal portable Markdown rather than a private code-message type. Fences remain transport/source syntax and are removed from the displayed conversation block by `MarkdownFormatter`.

This keeps the composer compatible with future ChatGPT/SaaS responses and with plain Markdown persistence.

A later renderer semantic pass can add monospace font-family metadata, code-block chrome, Copy buttons, language labels, syntax highlighting, and emoticon suppression inside code spans without changing this canonical representation.

## Composition scrollbars

The message-entry surface now reserves classic horizontal and vertical scrollbars around the editable viewport. This is necessary because the multiline composer intentionally does not word-wrap long logical lines yet and can also contain more lines than fit vertically.

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

## Validation checklist

Before marking this tranche target-validated:

1. Verify the toolbar shows the `<code />` toggle immediately after the emoticon control.
2. Verify the indentation selector is a native Windows combo box with 2/4/6/8 and defaults to 4.
3. Toggle code mode with the mouse and with `Ctrl+;`; verify the shortcut toggles once and does not insert `;`.
4. In code mode, verify Enter and Shift+Enter both create new lines and do not send.
5. Verify Ctrl+Enter sends from code mode.
6. Verify Tab inserts exactly 2/4/6/8 spaces according to the combo setting.
7. Send a code-mode message and verify conversation presentation treats it as a Markdown fenced code block.
8. Verify the composition area uses the native Server 2003/MiniXP horizontal and vertical scrollbar controls.
9. Type a logical line wider than the input viewport and use the horizontal scrollbar arrows, track, and thumb.
10. Create enough lines to overflow vertically and exercise the vertical scrollbar arrows, track, and thumb.
11. Verify caret placement and drag selection remain correct after scrolling on both axes.
12. Verify automatic caret-follow scrolling keeps keyboard editing visible near the right/bottom edges.
13. Verify Undo/Redo, clipboard operations, formatting, list mode, attachments, and emoticon insertion do not regress.
14. Repeat the validation under Windows Server 2003 SP2 and MiniXP.
