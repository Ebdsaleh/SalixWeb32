# Composer Formatting and Message Draft Model

This document records the post-`v0.0.2` composer architecture currently awaiting target validation.

## Native font-size control

The framework `ComboBox` remains backend-neutral, while the Win32 host may attach a native `COMBOBOX` peer. On Windows this provides the real system drop-down arrow, focus behavior, keyboard navigation, and `CBN_SELCHANGE` notification path instead of drawing a textual `v` analogue.

## Formatting state

The composer toolbar exposes Bold, Italic, Underline, and point-size state. The editable `TextInput` stores canonical text plus per-character `TextFormat` values. Formatting can therefore be applied either to selected ranges or to subsequently typed characters.

Undo/Redo continues to snapshot the text, selection state, and formatting state together.

## FormattedText

`FormattedText` is the portable snapshot used to move formatted content out of an editor without exposing `TextInput` internals. It stores canonical text plus aligned per-character `TextFormat` values.

The representation is intentionally simple for the short legacy-target message sizes. A future transport/serialization layer may compact adjacent identical character formats into runs without changing the public message semantics.

## MessageDraft

`MessageDraft` is now the composer-to-application payload. It contains:

```text
MessageDraft
    +-- FormattedText body
    +-- attachment path(s)
```

This prevents the shell from reconstructing a message after submit and gives a future network/backend layer one structured object to consume.

## Conversation preservation

`ConversationView` accepts formatted local/remote message bodies. The role prefix (`You:`, `Remote:`, `System:`) receives ordinary default formatting and the message body retains its original Bold/Italic/Underline/font-size formatting.

Conversation labels are still selectable/read-only. Formatted text measurement is used for caret placement and selection hit-testing so larger or bold text does not break pointer-to-character mapping.

Conversation rows derive their height from the largest font used by each message rather than assuming that every message fits a fixed 24-pixel row.

## Canonical and graphical emoticons

Classic aliases such as `:)`, `:D`, `;)`, `:P`, `:'(`, `<3`, and `<:` remain canonical text in `FormattedText` and `MessageDraft`. This keeps copy/paste and future transport interoperable.

The Win32 presentation layer may now replace those aliases visually with original classic-messenger-inspired GDI drawings while the underlying characters remain untouched. The composer and conversation history share the same framework registry and formatted hit-testing path.

See `docs/EMOTICON_RENDERING.md` for the rendering, selection, alias, and target-validation contract.

## Validation state

The composer/rich-message and graphical-emoticon tranche is not yet recorded as target-validated. Visual C++ 7.1 builds and runtime behavior on Windows Server 2003 SP2 and MiniXP remain authoritative.
