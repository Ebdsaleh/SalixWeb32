# Graphical Emoticon Rendering

This document records the post-`v0.0.2` graphical emoticon tranche. Target validation on Visual C++ 7.1 / Windows Server 2003 SP2 / MiniXP is still pending.

## Canonical message content

SalixWeb32 keeps classic text aliases as the canonical stored and transported representation:

```text
:)
:D
;)
:P
:'(
:O
<3
<:
```

The renderer may present those aliases as graphical emoticons, but copy/paste, message drafts, future transport, logging, and persistence continue to see the original text. No image bytes replace the alias in message content.

A few familiar colon-dash spellings such as `:-)`, `:-D`, `;-)`, `:-P`, `:-O`, and `:'-(` are accepted as visual aliases as well. The picker continues to insert the compact canonical spellings.

## Framework registry

The alias catalogue lives in `framework/EmoticonRegistry` so editable text, selectable labels, platform text metrics, the picker, and renderers can share one source of truth without making the Win32 renderer depend on application-layer code.

There is intentionally only one compiled `EmoticonRegistry.cpp` translation unit. Visual C++ 7.1 writes object files by source basename into the shared intermediate directory, so keeping both an application-layer and framework-layer `EmoticonRegistry.cpp` caused the framework implementation to be omitted from the VC7.1 build and produced unresolved linker symbols. `EmojiPanel` now includes `framework/EmoticonRegistry.h` directly.

## Win32 presentation

`Win32EmoticonPainter` draws an original classic-messenger-inspired set using ordinary GDI primitives. It does not embed or redistribute Microsoft/MSN/Windows Live Messenger artwork.

The current visual set covers:

- smile
- big grin
- wink
- tongue
- crying
- surprised
- heart
- classic / quirky face

The drawings intentionally use bright late-2000s messenger-style shapes and highlights while remaining SalixWeb32-owned rendering code.

## Inline replacement

`Win32TextPainter` scans canonical formatted text while painting. When an alias is encountered, the source characters occupy one visual inline-emoticon cell. The underlying character positions are retained.

This applies to both:

```text
TextInput        -> live composer preview
Label            -> sent conversation history
```

Formatting still applies. A larger point size produces a larger inline emoticon cell, and conversation rows already size themselves from the message formatting.

## Selection, caret and copy semantics

Win32 formatted hit-testing measures graphical emoticon cells rather than the hidden alias glyph widths. A mouse click on the left or right half of an emoticon maps to the source position before or after the complete alias, avoiding pointer drift in mixed text/emoticon messages.

Selections continue to operate on canonical source text. If any source character belonging to a displayed emoticon is selected, the visual cell receives the standard selection background. Copying the selection yields the original alias text.

Keyboard editing remains source-text editing: the alias is not converted into an opaque binary object. This is intentional because the canonical representation must remain portable and editable.

## Picker presentation

Existing picker buttons retain their alias text internally. The Win32 button renderer recognizes exact registered emoticon aliases and paints the matching graphical face instead of drawing the literal characters. The toolbar `:)` button therefore becomes graphical through the same catalogue without adding a separate application-only image table.

## Legacy constraints

The implementation uses GDI only. It does not require GDI+, Direct2D, a modern emoji font, PNG decoding, SVG, or a new runtime dependency. This keeps the first graphical implementation suitable for the Pentium 4 / NT 5.x target.

## Validation checklist

Before marking this tranche validated, test at minimum:

1. Type every canonical alias directly into the composer and confirm it becomes graphical while editing.
2. Insert every emoticon from the picker and confirm the same visual result.
3. Send mixed formatted text containing multiple emoticons and confirm the graphics survive in conversation history.
4. Select/copy a sent graphical emoticon and confirm the pasted text is the original alias.
5. Click immediately before and after several emoticons and confirm caret placement follows the visual cell.
6. Test discontinuous and subtractive selections across text plus emoticons.
7. Test 8pt through 24pt formatting around emoticons.
8. Resize and scroll a conversation containing many emoticons and mixed font sizes.
9. Repeat under both Windows Server 2003 SP2 and MiniXP.
