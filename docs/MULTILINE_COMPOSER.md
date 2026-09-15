# Multiline Composer and List Editing

This document records the post-`v0.0.2` multiline composer tranche. Visual C++ 7.1 / Windows Server 2003 SP2 / MiniXP target validation is still pending.

## Composer input model

`TextInput` now supports a multiline mode while retaining the existing canonical text, per-character `TextFormat`, discontinuous `TextSelection`, clipboard, cut/paste modes, and bounded Undo/Redo history.

The message composer enables multiline mode and expands the input surface to show several explicit lines at once.

Canonical line breaks are stored as `\n` characters. Carriage returns from pasted CRLF text are normalized away, and plain single-line `TextInput` instances continue to flatten line breaks to spaces.

## Enter behavior

The messenger interaction remains send-oriented:

```text
Enter        -> send the current draft
Shift+Enter  -> insert a line break
```

This keeps the quick chat workflow while making multiline drafting available without adding a separate mode switch.

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

The previously disabled `List` toolbar button is now active and opens a dedicated `ListPanel` with:

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

1. Enter sends while Shift+Enter creates several lines in the composer.
2. Up/Down, Home/End, Ctrl+Home/Ctrl+End and Shift-selection behave across lines.
3. B/I/U/font-size formatting survives across multiple lines and survives Send.
4. Graphical emoticons render correctly on several different lines.
5. Select multiple lines and apply Bullets; verify each touched line receives one `* ` prefix.
6. Apply Bullets again and verify the prefixes are removed.
7. Apply Numbered to several lines and verify sequential `1.`, `2.`, `3.` prefixes.
8. Switch selected numbered lines to Bullets and verify prefixes are replaced rather than stacked.
9. Ctrl+Z/Ctrl+Y undo and redo one entire list transformation atomically.
10. Send a multiline/list message and verify conversation row height, selection, copy, and emoticon hit-testing.
11. Repeat the validation under both Windows Server 2003 SP2 and MiniXP.
