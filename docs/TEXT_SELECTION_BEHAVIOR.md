# Text Selection and Clipboard Behavior

This document records the intended and target-validated interaction semantics for SalixWeb32 text selection and clipboard operations.

## Target-validated behavior

The discontinuous-selection and paste-mode implementation from:

```text
ad147f1 Add discontinuous text selection and paste modes
```

was rebuilt and exercised successfully on the Pentium 4 / Windows Server 2003 target with Visual C++ 7.1.

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

## Interaction note

The currently validated additive mouse workflow is therefore:

```text
select range
    -> Ctrl+click to relocate the caret without clearing existing highlighting
    -> Shift+drag to select another section while retaining the existing range(s)
```

This behavior is accepted as the current framework interaction model. Future changes to additive-selection gestures should preserve the underlying `TextSelection` multi-range model and clipboard semantics even if the exact mouse gesture is refined.

## Design boundary

Discontinuous selections belong to the shared framework text-selection layer rather than to the messenger shell. Clipboard export continues to provide an interoperable compact `text/plain` representation plus a Salix-specific preserved-layout representation for paste options.
