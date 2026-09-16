# Multiline Composer and List Editing

This document records the post-`v0.0.2` multiline composer tranche. Visual C++ 7.1 / Windows Server 2003 SP2 / MiniXP target validation is still pending.

## Composer input model

`TextInput` supports a multiline mode while retaining the existing canonical text, per-character `TextFormat`, discontinuous `TextSelection`, clipboard, cut/paste modes, and bounded Undo/Redo history.

The message composer enables multiline mode and expands the input surface to show several explicit lines at once.

Canonical line breaks are stored as `\n` characters. Carriage returns from pasted CRLF text are normalized away, and plain single-line `TextInput` instances continue to flatten line breaks to spaces.

## Enter behavior

The messenger interaction remains send-oriented outside lists and code mode:

```text
Enter        -> send the current draft
Shift+Enter  -> insert a line break
Ctrl+Enter   -> send the current draft
```

When the caret is on a recognized bulleted or numbered list line, the List toolbar button becomes visually checked and `Enter` changes meaning from Send to Continue List:

```text
* first item| + Enter

becomes

* first item
* |
```

Numbered lines increment from the current visible prefix:

```text
7. seventh item| + Enter

becomes

7. seventh item
8. |
```

While the caret remains on a recognized list line:

```text
Enter        -> insert a newline plus the next list prefix
Shift+Enter  -> insert a plain newline without a new list prefix
Ctrl+Enter   -> send the current draft
```

Code mode has its own Enter/Tab semantics and is documented in `docs/CODE_COMPOSER.md`.

## List-state indicator

The toolbar List control is a real `ToggleButton`, but its checked state is not merely whether the popup is open. It reflects the list state of the logical line containing the caret.

The composer synchronizes this state after editing and navigation. Moving the caret from a list line to an ordinary line therefore releases the List toggle; moving back to a recognized `* `, `- `, or `N. ` line checks it again. This also means manually typed canonical list prefixes participate in the same behavior.

Internally the composer keeps the exact style (`clear`, `bulleted`, or `numbered`) even though the toolbar only needs a checked/unchecked visual state.

## Navigation

Multiline editing adds:

- Up / Down moves the caret between logical lines while preserving the source-column position where possible.
- Shift+Up / Shift+Down extends the current selection vertically.
- Home / End moves to the start/end of the current logical line.
- Ctrl+Home / Ctrl+End moves to the start/end of the complete document.
- Existing Left/Right, Ctrl+Left/Right, mouse selection, additive selection, and subtractive selection continue to operate on the same source-text model.

## Formatting across lines

Bold, Italic, Underline, and point-size formatting continue to use per-character `TextFormat` values. Line-break characters remain structural rather than visible formatted glyphs.

A sent multiline `MessageDraft` keeps both its line breaks and inline formatting. `ConversationView` allocates taller rows for multiline messages so sent content does not overlap neighboring history entries.

The Win32 text painter and hit-testing path understand explicit lines, mixed font sizes, and inline graphical emoticons.

## List popup

The List toolbar button opens a dedicated `ListPanel` with:

```text
Bullets
Numbered
Clear list
```

The list operation applies to the current logical line when there is no selection, or to every logical line touched by the current selection ranges.

Bulleted lists use the portable canonical prefix:

```text
* item
```

Numbered lists use:

```text
1. first
2. second
3. third
```

Applying the same list style to lines that already use that style toggles the prefixes off. Switching between bullet and numbered modes replaces the recognized existing list prefix instead of stacking prefixes.

List operations are a single Undo/Redo edit transaction.

## Canonical representation

Lists remain ordinary message text at this stage. This is intentional: copy/paste, future transport, persistence, and non-rich peers can all understand the canonical `* ` / `N. ` representation without a private paragraph object.

A later rich-document layer may add semantic paragraph metadata while preserving this portable text fallback.

## Composer viewport and scrollbars

The composer now reserves horizontal and vertical scrollbars around the editable text viewport.

The vertical scrollbar exposes drafts with more logical lines than fit in the visible input area. The horizontal scrollbar exposes long logical lines because automatic word wrapping is not enabled yet.

The scrollbars support arrow-step movement, page-step track clicks, and draggable thumbs. Scroll offsets are presentation state only; they do not modify the canonical text, formatting, selection ranges, Undo/Redo history, or submitted message payload.

Mouse hit-testing is translated through the same viewport offsets used by the Win32 input painter so caret placement and drag selection continue to address source positions after scrolling.

## Current limits

Automatic word wrapping is still intentionally deferred. Long logical lines are navigated with the new horizontal scrollbar rather than being wrapped.

Conversation-history wrapping and richer Markdown block widgets are separate presentation follow-ups; the scrollbar work in this tranche is specifically for the composition input surface.

## Validation checklist

Before marking this tranche validated, test at minimum:

1. Outside a list/code mode, Enter sends while Shift+Enter creates several lines in the composer.
2. Ctrl+Enter submits the draft directly.
3. On a bulleted line, Enter creates the next `* ` item and keeps the List toolbar control checked.
4. On a numbered line, repeated Enter increments `1.`, `2.`, `3.` and so on.
5. Move the caret between list and ordinary lines and verify the List toggle follows the current line.
6. While on a list line, Ctrl+Enter sends rather than creating another item.
7. Up/Down, Home/End, Ctrl+Home/Ctrl+End and Shift-selection behave across lines.
8. B/I/U/font-size formatting survives across multiple lines and survives Send.
9. Graphical emoticons render correctly on several different lines.
10. Select multiple lines and apply Bullets; verify each touched line receives one `* ` prefix.
11. Apply Bullets again and verify the prefixes are removed.
12. Apply Numbered to several lines and verify sequential `1.`, `2.`, `3.` prefixes.
13. Switch selected numbered lines to Bullets and verify prefixes are replaced rather than stacked.
14. Ctrl+Z/Ctrl+Y undo and redo one entire list transformation atomically.
15. Overflow the composer horizontally and vertically and exercise both scrollbars.
16. Verify caret placement and mouse selection after scrolling.
17. Send a multiline/list message and verify conversation row height, selection, copy, and emoticon hit-testing.
18. Repeat the validation under both Windows Server 2003 SP2 and MiniXP.
