# Text Encoding Boundary

SalixWeb32 stores application/framework text as UTF-8 bytes and converts to native
UTF-16 only at the Win32 presentation and clipboard boundary.

This policy keeps service/network/storage contracts byte-oriented and portable while
using the Unicode-capable Win32 APIs that are available on the Windows Server 2003
target.

## Canonical path

```text
ChatGPT / modern browser DOM
        |
        | JavaScript Unicode string
        v
WebExtension JSON
        |
        | UTF-8
        v
salix_chat_session.py
        |
        | UTF-8
        v
salix_bridge.py
        |
        | SALIX-CONVERSATION/1 UTF-8 event payloads
        v
RemoteConversationBackend
        |
        | std::string UTF-8 bytes
        v
FormattedText / Label / TextInput
        |
        | UTF-8 byte storage + code-point-aware boundaries
        v
Win32 text backend
        |
        | MultiByteToWideChar(CP_UTF8)
        v
UTF-16
        |
        +-- GetTextExtentPoint32W
        +-- TextOutW
        +-- DrawTextW
        +-- CreateFontW
        `-- CF_UNICODETEXT
```

## Framework index model

The existing framework keeps text and formatting in byte-indexed `std::string` storage.
This tranche does not replace that model with `std::wstring`.

Instead, UTF-8 helpers guarantee that interactive/layout boundaries do not land inside a
valid multibyte sequence.

The following operations are code-point boundary aware:

- soft wrapping,
- formatted hit testing,
- left/right caret navigation,
- delete/backspace,
- label selection endpoints,
- pasted-text truncation.

Per-byte formatting arrays remain compatible because every byte belonging to one UTF-8
code point receives the same text format in the normal framework paths.

## Win32 presentation boundary

Win32 renderers must not pass UTF-8 bytes to ANSI GDI text APIs.

The formatted text path therefore uses:

```text
GetTextExtentPoint32W
TextOutW
DrawTextW
CreateFontW
```

`Win32Utf8Text` performs bounded UTF-8 -> UTF-16 conversion for each measured/drawn
span.

For compatibility with older Salix strings that still originate from Win32 ANSI APIs,
presentation uses UTF-8 when the source bytes form valid UTF-8 and falls back to the
current Windows ANSI code page only when they do not. This is a transitional
compatibility rule, not permission for new application/service text to use ACP.

## Clipboard boundary

The standard Salix text MIME payload remains UTF-8.

On Win32:

- copy publishes `CF_UNICODETEXT`,
- a best-effort `CF_TEXT` flavor is generated from Unicode for legacy external apps,
- paste prefers `CF_UNICODETEXT` and converts UTF-16 -> UTF-8,
- legacy `CF_TEXT` paste converts ACP -> UTF-8,
- the private Salix preserved-selection payload remains UTF-8 bytes.

This allows copied assistant text to preserve smart punctuation and other Unicode
characters when exchanged with Unicode-aware Server 2003 applications.

## Native character input

The current application window remains an ANSI Win32 window for compatibility with the
existing shell.

`WM_CHAR` high bytes are normalized from the active ANSI code page to a Unicode code
point at the application-host boundary and then encoded into UTF-8 before entering
`TextInput`.

This improves direct legacy-keyboard input without requiring the whole application window
class to be converted to Unicode in the same tranche.

DBCS/IME-heavy input remains a future platform-input concern and is not the validation
gate for this tranche.

## Font coverage is separate from encoding

Correct UTF-8/UTF-16 handling must be paired with Win32 font fallback.

Tahoma and Courier New remain the preferred Salix faces, but they are not assumed to
cover every Unicode code point that the target OS can draw. When a preferred face lacks
one or more glyphs in a rendered span, Salix probes cached fallback faces such as Lucida
Sans Unicode, Lucida Console, Arial Unicode MS, and Arial while preserving the requested
size/style. Measurement and drawing use the same selected fallback font.

The first target pass rendered `→ ↓` as boxes in Salix while Server 2003 Notepad
displayed the same underlying characters correctly. That proved the target OS had usable
glyph coverage and narrowed the remaining bug to Salix font selection.

The corrected path probes fallback faces when the preferred font lacks one or more
glyphs in a span. The real-P4 retest then rendered `→ ↓` correctly in both Conversation
presentation and the composer. VC7.1 compatibility is preserved by resolving
`GetGlyphIndicesW` dynamically from `gdi32.dll` instead of depending on newer SDK
header declarations.

Mojibake such as:

```text
Iâ€™m
```

is an encoding failure.

A missing-glyph box for a correctly decoded code point is a font-coverage limitation.

## Validation

See `docs/VALIDATION.md`.

The real-target Unicode gate requires:

```text
I’m “testing” — café € → ↓
```

to survive:

```text
ChatGPT
 -> relay
 -> native Salix rendering
 -> drag selection
 -> CF_UNICODETEXT copy
 -> Server 2003 Notepad
 -> paste back into Salix composer
 -> relay back to the browser thread
```

with no mojibake.

The first real-P4 pass confirmed that the phrase round-tripped back through the browser
relay byte-for-byte as Unicode text. Server 2003 rendered the smart punctuation, Latin
accent, euro sign, and em dash correctly. The arrow code points were preserved through
native selection/copy/paste and arrived intact back at ChatGPT.

After glyph-aware fallback was added, the final real-P4 retest rendered `→ ↓`
correctly inside SalixWeb32 as well. The matching VC7.1 rebuild completed with zero
errors and zero warnings. The encoding, clipboard, selection, composer, relay, and native
font-fallback paths are therefore validated together.
