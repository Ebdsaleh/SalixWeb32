# Native Context Menus and Read-only Conversation Selection

This document records the post-v0.0.2 context-menu and conversation-selection tranche. The implementation is designed to feel like a native Win32 application while keeping menu contents and text-edit commands backend-neutral above the platform layer.

## Context-menu architecture

`framework/ContextMenu.h` is a lightweight backend-neutral popup-menu model. A menu contains command items, separators, labels, and enabled/disabled state; it does not contain Win32 handles or command-routing code.

`NativeControlHost` exposes the presentation contract:

```text
show_context_menu(menu, client_x, client_y) -> command id
```

The Win32 implementation uses the operating system's popup-menu APIs (`CreatePopupMenu`, `AppendMenuA`, and `TrackPopupMenu`) so Server 2003 and MiniXP receive their native menu appearance rather than a framework-painted approximation.

`Win32ApplicationHost` translates a right-button release into `UIEvent::event_context_menu`, supplies the existing clipboard and text-metrics services, and exposes the active native-control host through the event. Components decide which menu commands are meaningful; the Win32 layer only presents the resulting menu and returns the selected command id.

## Composer edit menu

Right-clicking inside the editable message viewport presents:

```text
Copy
Cut
Cut - Keep Formatting
Paste
Paste - Keep Formatting
-----------------------
Select All
```

The commands intentionally reuse the same editing semantics as the established keyboard paths:

```text
Copy                     -> Ctrl+C semantics
Cut                      -> Ctrl+X semantics
Cut - Keep Formatting    -> Ctrl+Shift+X semantics
Paste                    -> Ctrl+V semantics
Paste - Keep Formatting  -> Ctrl+Shift+V semantics
Select All               -> full draft selection
```

Copy/Cut commands are disabled when there is no selection. Paste commands are disabled when the clipboard does not expose a supported text payload. Select All is disabled for an empty draft.

The popup does not create a second implementation of cut/copy/paste. It dispatches through the existing `TextInput` editing paths, so Undo/Redo history, compact/preserved clipboard behavior, formatting preservation, caret-follow scrolling, and scrollbar extent updates continue to use one source of truth.

## Read-only conversation context menu

A block-composed conversation message now treats its visible text surfaces as one read-only presentation group. This includes:

- the separate role header when present,
- wrapped prose/Markdown labels,
- selectable code text inside `CodeBlockView`.

Right-clicking the message presentation exposes:

```text
Copy
-----------------------
Select All
```

Copy gathers every currently selected range in presentation order. When a selection spans multiple presentation blocks, block boundaries are copied as line breaks rather than flattening unrelated prose/code fragments into one word stream.

Select All selects the complete visible text presentation for that message, including the role header where it is rendered separately and code text where present.

## Cross-block drag selection

A normal single-click drag can now cross the internal block boundaries of one conversation message. The selection coordinator operates above the individual `Label` controls:

```text
role header
    -> prose block
    -> code block text
    -> prose block
```

The initial click records an anchor label and character position. As the pointer crosses later or earlier blocks, the coordinator:

1. selects the remainder of the anchor block,
2. fully selects intermediate blocks,
3. selects the appropriate prefix/suffix of the current endpoint block.

Dragging backward uses the symmetric behavior. Gaps between blocks resolve to the nearest adjacent text surface, so crossing the spacing between a paragraph and code block does not abruptly cancel the selection.

Existing advanced gestures remain owned by individual text controls. Double/triple click, Ctrl-based additive selection, Ctrl+Alt subtractive selection, and Shift selection are therefore not replaced by the cross-block single-drag coordinator.

The current grouping boundary is one `ConversationMessageView`. A single drag does not yet continue through a second message row; that is a future conversation-document selection refinement rather than being silently faked in this tranche.

## Clipboard behavior

Editable composer menus continue to expose both compact and formatting-preserving cut/paste modes through the existing private Salix clipboard MIME payload.

Read-only conversation Copy emits ordinary text in presentation order. It never mutates the underlying canonical Markdown/message data.

Code-block source remains literal. Graphical emoticon presentation does not rewrite copied code characters.

## Target validation checklist

This tranche is not target-validated until exercised on the real legacy systems.

1. Rebuild under Visual C++ 7.1 with no new warnings/errors.
2. Right-click inside the composer and confirm a real native Windows popup menu appears.
3. Verify Copy/Cut are disabled with no selection and enabled with a selection.
4. Verify Paste items are disabled when no supported clipboard text exists.
5. Verify ordinary Cut/Paste match Ctrl+X/Ctrl+V behavior.
6. Verify Keep Formatting Cut/Paste match Ctrl+Shift+X/Ctrl+Shift+V behavior.
7. Verify Select All selects the complete draft.
8. Verify menu operations preserve Undo/Redo behavior and update caret/scroll extents after mutations.
9. Drag-select a wrapped prose block in conversation history and right-click Copy.
10. Drag from a prose block through a code block into later prose in the same message; all crossed text surfaces should highlight continuously.
11. Drag the same mixed message backward and verify the symmetric selection behavior.
12. Verify the separate `You:`/`Remote:` role header can participate in selection when block layout separates it from content.
13. Right-click a multi-block selection and verify Copy produces text in presentation order with line breaks between selected blocks.
14. Use Select All on a mixed prose/code message and verify the complete visible text presentation is selected.
15. Verify the existing code-block `Copy` button still copies only raw code.
16. Verify double/triple click and additive/subtractive selection gestures still work inside individual text surfaces.
17. Verify native scrollbar behavior and mouse-wheel scrolling do not regress while popup menus are used.
18. Validate first on Windows Server 2003 SP2, then repeat the smoke pass under MiniXP.
