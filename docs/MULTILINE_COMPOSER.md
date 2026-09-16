# Multiline Composer and List Editing

This document records the post-`v0.0.2` multiline composer tranche. Visual C++ 7.1 / Windows Server 2003 SP2 / MiniXP target validation is still pending.

## Composer input model

`TextInput` now supports a multiline mode while retaining the existing canonical text, per-character `TextFormat`, discontinuous `TextSelection`, clipboard, cut/paste modes, and bounded Undo/Redo history.

The message composer enables multiline mode and expands the input surface to show several explicit lines at once.

Canonical line breaks are stored as `\n` characters. Carriage returns from pasted CRLF text are normalized away, and plain single-line `TextInput` instances continue to flatten line breaks to spaces.

## Enter behavior

The messenger interaction remains send-oriented outside lists:

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

This makes ordinary Enter useful for rapid list entry without sacrificing a direct keyboard Send command. The canonical document still stores ordinary `\n`, `* `, and `N. ` text rather than hidden paragraph objects.

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

A sent multiline `MessageDraft` keeps both its line breaks and inline formatting. `ConversationView` now allocates taller rows for multiline messages so sent content does not overlap neighboring history entries.

The Win32 text painter and hit-testing path both understand explicit lines, mixed font sizes, and inline graphical emoticons.

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

## Current limits

This tranche handles explicit line breaks, not automatic word wrapping. Long logical lines are clipped horizontally by the composer surface. The composer also does not yet expose an internal scrollbar; very large drafts can extend below the visible editing area even though the full text remains in the model.

Those are presentation/viewport follow-ups rather than blockers for validating multiline editing, formatting, lists, transport payloads, and conversation preservation.

## Validation checklist

Before marking this tranche validated, test at minimum:

1. Outside a list, Enter sends while Shift+Enter creates several lines in the composer.
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
15. Send a multiline/list message and verify conversation row height, selection, copy, and emoticon hit-testing.
16. Repeat the validation under both Windows Server 2003 SP2 and MiniXP.
