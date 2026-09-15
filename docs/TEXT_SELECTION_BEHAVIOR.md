# Text Selection and Clipboard Behavior

This document records the intended and target-validated interaction semantics for SalixWeb32 text selection and clipboard operations.

## Target-validated behavior

The discontinuous-selection and paste-mode implementation from:

```text
ad147f1 Add discontinuous text selection and paste modes
```

was rebuilt and exercised successfully on the Pentium 4 under both Windows Server 2003 SP2 and MiniXP with Visual C++ 7.1.

Observed behavior:

- Normal click places the caret.
- Normal drag selects a contiguous range.
- Double-click selects a word.
- Triple-click selects the complete current single-line value.
- Ctrl+click moves the caret to the clicked position while preserving the existing highlighted range(s).
- After preserving an existing range with Ctrl+click, holding Shift while drag-selecting another section can add another highlighted section while retaining the previous highlighting.
- Multiple selected ranges can therefore remain visible at the same time.
- Ctrl+C serializes multiple selected fragments in source order.
- Normal paste uses the compact `text/plain` representation, joining selected fragments with a single separator space.
- Ctrl+Shift+V uses the keep-formatting path and preserves the original spacing between selected fragments by replacing unselected characters in the preserved span with spaces.

Example source text:

```text
Hello Pentium Hello Pentium
      [Pentium]     [Pentium]
```

Compact/default paste:

```text
Pentium Pentium
```

Keep-formatting paste:

```text
Pentium       Pentium
```

## Validation-photo spacing note

During target testing, one screenshot showed an additional manually entered space between two pasted `Pentium` fragments. That space was deliberately typed by the tester and is not evidence of a clipboard or preserved-spacing mismatch.

## Validated additive mouse workflow

The currently validated additive mouse workflow is:

```text
select range
    -> Ctrl+click to relocate the caret without clearing existing highlighting
    -> Shift+drag to select another section while retaining the existing range(s)
```

This behavior is accepted as the current framework interaction model. Future changes to additive-selection gestures should preserve the underlying `TextSelection` multi-range model and clipboard semantics even if the exact mouse gesture is refined.

## Validated subtractive selection

The subtractive-selection tranche from:

```text
0cc832e Add subtractive text selection gestures
```

has been exercised successfully on the Pentium 4 under Windows Server 2003 SP2 and MiniXP.

```text
Ctrl+Alt+drag
```

removes the dragged character span from the current selection set. Only overlapping highlighted characters are removed; text outside the existing selection remains unaffected. If the dragged span cuts through the middle of one highlighted range, that range is split into two independent ranges.

```text
Ctrl+Alt+double-click
```

removes the clicked word from the current highlighted ranges. This uses the same whitespace-delimited word boundaries as normal double-click word selection.

For symmetry with the existing triple-click behavior, Ctrl+Alt+triple-click removes the clicked logical line from the current selection set. The current controls are single-line, so this presently means the complete text value.

Subtractive gestures operate in both editable `TextInput` controls and selectable/read-only `Label` controls. They change selection state only; they do not delete or edit the underlying text.

## Cut modes and edit history target

The next text-input tranche adds two explicit cut modes:

```text
Ctrl+X
```

performs the normal compact cut. The selected ranges are copied using the existing dual clipboard representations and then physically removed from the editable text, causing the surrounding text to collapse together in the normal editor fashion.

```text
Ctrl+Shift+X
```

performs a keep-formatting cut. The same selection is copied to the clipboard, but each removed source character is replaced with a normal space character instead of collapsing the surrounding text. The source string therefore retains its character positions and visual spacing.

Example:

```text
before:  The [quick] brown [fox] jumps
normal:  The  brown  jumps
keep:    The         brown       jumps
```

The same tranche introduces bounded undo/redo history for editable `TextInput` state:

- Ctrl+Z performs Undo.
- Ctrl+Y performs Redo.
- Ctrl+Shift+Z is also accepted as Redo.
- Undo/redo snapshots restore both text and the full `TextSelection` state, including discontinuous ranges and caret position.
- A fresh edit after Undo clears the redo branch.
- Paste and either cut mode are atomic history operations.
- Consecutive character typing is grouped into one undo operation until navigation/selection or another edit class breaks the group.
- Repeated Backspace is grouped into one undo operation.
- Repeated Delete is grouped into one undo operation.
- The history is bounded to avoid unbounded memory growth on the legacy target.

These cut and history behaviors are pending target validation on VC7.1 / Server 2003 and MiniXP before the `v0.0.2` baseline is tagged.

## Design boundary

Discontinuous selections belong to the shared framework text-selection layer rather than to the messenger shell. Clipboard export continues to provide an interoperable compact `text/plain` representation plus a Salix-specific preserved-layout representation for paste options. Undo/redo history belongs to the editable text control because it records mutations rather than read-only selection state.
