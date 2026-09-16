# Post-v0.0.2 Target Validation

This document supplements `docs/VALIDATION.md` with target observations for the active composer/conversation work after the tagged `v0.0.2` text-editing baseline.

The post-baseline work remains experimental until each tranche is exercised on the intended legacy targets. A successful test on Windows Server 2003 must not be silently treated as MiniXP validation, and vice versa.

## Target environment

Primary observations in this document come from the real Pentium 4 development target:

```text
CPU:        Pentium 4 class
Memory:     2 GB
OS:         Windows Server 2003 SP2 x86
Compiler:   Visual C++ 7.1 / Visual Studio .NET 2003
```

MiniXP remains the secondary smoke-test environment on the same hardware.

## Graphical emoticons

After the VC7.1 same-basename object-file collision was corrected, the real target successfully rebuilt and displayed the graphical classic-emoticon implementation.

Observed on the target:

- graphical classic-style emoticons render in conversation history,
- the toolbar/emoji presentation renders graphical faces,
- canonical aliases remain the underlying message data.

Detailed interaction coverage remains recorded in `docs/EMOTICON_RENDERING.md`.

## Multiline and list editing

The target has displayed and exercised multiline/list composition including continued numbered/bulleted lines. Subsequent list behavior introduced context-sensitive Enter semantics and `Ctrl+Enter` forced send.

Observed examples include:

```text
* item
* next item
```

and:

```text
1. one
2. two
3. three
4.
```

The list state participates in the richer composer rather than being rendered as a one-off static demonstration.

## Markdown presentation

The Windows Server 2003 target successfully rebuilt and displayed Markdown-oriented conversation presentation.

Observed presentation included:

- role headers separated from block-style content,
- numbered list alignment beneath `You:`,
- headings/emphasis rendered through formatted text,
- normal text and graphical emoticons continuing to render.

## Code composer mode

The target successfully displayed the `<code />` toolbar control, indentation selector, multiline code input, and code submission path.

Observed behavior/presentation includes:

- code-mode content reaches conversation history,
- code text is visibly distinguished from ordinary text,
- Courier-style code presentation is active,
- source indentation survives submission,
- code aliases such as `<3` remain literal where code semantics are active,
- ordinary text can still render the graphical heart/emoticon presentation.

The target screenshots therefore confirm that normal and code semantic paths are visibly distinct.

## Legacy mouse-wheel compatibility

The native conversation scrolling tranche initially failed to compile because the legacy SDK headers did not expose `WM_MOUSEWHEEL`.

The compatibility fallback:

```cpp
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
```

was added without globally raising the Win32 target version. The user rebuilt after the fix and reported that it works.

This confirms the compatibility fix on the primary Server 2003/VC7.1 target.

## Wrapped rich conversation viewport

The wrapped/pixel-scrolled conversation build has successfully compiled and run on the Windows Server 2003 target. Screenshots confirm the richer normal-vs-code presentation operating inside the scrollable conversation surface.

The following full checklist is still pending before calling the tranche completely validated:

- very long ordinary paragraph soft wrapping,
- copy of soft-wrapped prose proving no new canonical line breaks were inserted,
- repeated narrow/wide resize reflow,
- one response taller than the whole viewport,
- selection across several soft-wrapped lines,
- exhaustive native scrollbar/thumb/wheel behavior with huge messages,
- equivalent MiniXP smoke coverage.

See `docs/RICH_CONVERSATION_VIEWPORT.md`.

## Current pending tranche: block-composed code presentation

The newest tranche replaces flattened fenced-code presentation with a block-composed message tree:

```text
ConversationMessageView
    +-- role header
    +-- wrapped text Label
    +-- CodeBlockView
    +-- wrapped text Label
```

Target validation must confirm:

- Visual C++ 7.1 compiles and links the new source files,
- prose/code/prose in one canonical Markdown message becomes distinct blocks,
- dedicated code background/border presentation replaces disconnected per-line bands,
- the fence language token appears in the code-block header,
- the Copy button places only raw code body text on the clipboard,
- code text remains selectable with Ctrl+C,
- emoticon aliases stay literal inside code and graphical outside it,
- long code lines remain unwrapped and can be moved with the code-block horizontal scrollbar,
- outer conversation vertical scrolling and inner code horizontal scrolling remain independent,
- partially visible code blocks remain clipped to the conversation viewport,
- resize does not corrupt code/text block geometry,
- multiple code blocks in one message remain stable.

After Windows Server 2003 validation, repeat a smoke test under MiniXP before marking this post-baseline tranche cross-target validated.

See `docs/CODE_BLOCKS.md` for the detailed contract.
