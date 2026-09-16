# Native Context Menus and Conversation Selection

This document records the post-v0.0.2 context-menu and conversation-selection work. The implementation is designed to feel like a native desktop application while keeping menu contents and text-edit commands backend-neutral above the platform layer.

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

## Conversation context is explicit

A generic `Select All` command inside conversation history proved ambiguous because the first implementation selected only the message instance under the pointer. That behavior was technically consistent but not sufficiently trustworthy for a desktop-style read-only document surface.

The conversation popup now makes scope explicit.

When the pointer is over a message row, the menu is shaped like:

```text
Copy
-----------------------
Select All in System Message
Select All Conversation
```

or:

```text
Copy
-----------------------
Select All in User Message
Select All Conversation
```

or:

```text
Copy
-----------------------
Select All in Remote Message
Select All Conversation
```

The role-specific item always refers to the message row under the pointer. The conversation-wide item always refers to the complete conversation document.

When the pointer is over conversation whitespace that is not associated with a message row, the role-specific item is omitted:

```text
Copy
-----------------------
Select All Conversation
```

This removes the previous ambiguity where a user could invoke `Select All` from whitespace without knowing which hidden message scope would be affected.

## Conversation-wide read-only selection

Selection coordination now lives one level above individual message rows in `ConversationView`.

A message may itself contain several selectable surfaces:

```text
role header
    -> prose block
    -> code block text
    -> prose block
```

and the conversation contains several such messages:

```text
System message
    -> User message
    -> Remote message
    -> User message
```

A normal drag can cross both kinds of boundary. The coordinator records:

- anchor message index,
- anchor presentation-label index,
- anchor character position.

As the pointer moves, it resolves the target message, target presentation label, and target character. It then:

1. selects the remainder of the anchor surface,
2. selects the remainder of the anchor message,
3. fully selects every intermediate message,
4. selects the required prefix of the target message,
5. selects the required prefix of the target surface.

Dragging backward applies the symmetric operation.

This means a drag can begin in `System:` text, continue through one or more `You:`/`Remote:` entries, cross prose/code/prose boundaries, and finish in a later message while remaining one logical read-only selection.

## Drag autoscroll

The Win32 host already captures the mouse during a left-button drag. While conversation-wide selection is active, moving above or below the visible conversation viewport scrolls the conversation by one line step per routed movement and extends the selection into newly revealed messages.

This is intentionally document-like behavior: a long conversation can be selected beyond the initially visible viewport without requiring the user to release the mouse and manipulate the scrollbar manually.

## Copy behavior

`Copy` gathers selected presentation text from every selected message in conversation order. Message boundaries are represented by line breaks. Within one block-composed message, block boundaries are also represented by line breaks.

The operation never mutates canonical Markdown or source code.

Code-block source remains literal. Graphical emoticon presentation does not rewrite copied code characters.

The existing code-block `Copy` button keeps its narrower meaning: it copies the complete raw code body for that one code block.

## Keyboard scope

After the conversation surface becomes the active selection context:

```text
Ctrl+C -> copy the complete current conversation selection
Ctrl+A -> select all conversation presentation text
```

Clicking outside the conversation deactivates that selection context so composer shortcuts remain owned by the composer.

## Target validation checklist

This refinement is not target-validated until exercised on the real legacy systems.

1. Rebuild under Visual C++ 7.1 with no new warnings/errors.
2. Right-click the composer and confirm its native edit menu still works unchanged.
3. Right-click a System message and confirm the menu says `Select All in System Message` plus `Select All Conversation`.
4. Right-click a User message and confirm the menu says `Select All in User Message` plus `Select All Conversation`.
5. Right-click a Remote message and confirm the menu says `Select All in Remote Message` plus `Select All Conversation`.
6. Right-click whitespace between/below messages and confirm only `Select All Conversation` is offered for selection scope.
7. Choose the role-specific command and verify only that one message presentation is selected.
8. Choose `Select All Conversation` and verify every visible and scrollable message presentation is selected.
9. Drag from a System message into a later User/Remote message and verify all crossed messages highlight continuously.
10. Drag backward from a later message into an earlier message and verify symmetric selection.
11. Drag through prose -> code -> prose inside one message and then continue into another message.
12. Copy a multi-message selection into Notepad and verify message/block ordering and line breaks are sensible.
13. With a conversation selection active, verify Ctrl+C copies the whole selection and Ctrl+A expands to the entire conversation.
14. Drag beyond the top/bottom viewport edge and verify autoscroll extends the selection into newly revealed history.
15. Verify the code-block `Copy` button still copies raw code only.
16. Verify double/triple click and Ctrl/Ctrl+Alt advanced gestures still work within individual selectable text surfaces.
17. Verify native scrollbar behavior and mouse-wheel scrolling do not regress while popup menus and drag-autoscroll are used.
18. Validate first on Windows Server 2003 SP2, then repeat the smoke pass under MiniXP.
