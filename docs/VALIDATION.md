# SalixWeb32 Target Validation

This document records validation performed on real legacy target environments.

## 2026-09-15 — Phase 1 native skeleton baseline

### Primary target

```text
CPU:        Pentium 4 class
Memory:     2 GB
OS:         Windows Server 2003 SP2 x86
Compiler:   Visual C++ 7.1 / Visual Studio .NET 2003
Executable: SalixWeb32.exe
```

Validated behavior:

- VS.NET 2003 solution opens successfully.
- Debug configuration compiles and links successfully with VC7.1.
- Native Win32 application launches successfully from Visual Studio.
- Native Win32 application launches successfully from Command Prompt.
- `ApplicationRuntime` initializes.
- `Win32ApplicationHost` creates and displays the application window.
- Win32 message loop remains responsive.
- Status text renders correctly.
- Normal window close exits cleanly without an observed crash or hung process.

Validated baseline commit:

```text
4628025 Add Phase 1 native Win32 runtime skeleton
```

### Secondary smoke test

The same built executable was launched successfully in a MiniXP environment on the same Pentium 4 system.

This is a portability smoke test only. It does not replace testing on a clean retail Windows XP installation and does not prove the absence of redistributable runtime dependencies.

## 2026-09-15 — Phase 1 hardening validation

The runtime hardening tranche was rebuilt and exercised on the Pentium 4 target.

Validated behavior:

- `RuntimeStatusService` starts through `ServiceRegistry`.
- Runtime update ticks advance continuously.
- Service count reports correctly.
- Client-area width and height update while resizing the window.
- Status content remains centered after resizing.
- Normal window close continues to exit cleanly.
- The executable continues to launch successfully from Command Prompt.

An initial repaint defect was found during validation: the changing update-tick text was drawn over stale pixels because the paint path used a transparent text background without clearing the client area first.

The defect was corrected by clearing the client area inside `WM_PAINT` before drawing status text. The updated build was then revalidated on the target system with the window left running and with repeated resizing. No status-text ghosting was observed after the fix.

Relevant commits:

```text
5547277 Harden Phase 1 runtime lifecycle and resize handling
0fd362e Fix Win32 status repaint ghosting
```

### Phase 1 result

Phase 1 exit criteria are satisfied on the primary target:

- native x86 build succeeds with Visual C++ 7.1,
- the runtime/service lifecycle operates,
- the Win32 host creates and maintains a responsive window,
- resize handling is functional,
- status rendering is stable,
- and shutdown is clean.

The validated Phase 1 baseline is tagged:

```text
v0.0.1
```

## 2026-09-15 — Phase 2 presentation-separation validation

The first Phase 2 tranche was rebuilt and run on the Pentium 4 target.

Validated commit:

```text
c3a3406 Begin Phase 2 framework presentation separation
```

Validated behavior:

- Visual C++ 7.1 accepts the new framework/presentation split.
- `StatusView` renders through the backend-neutral `View` contract.
- status text is represented by framework `Label` components.
- Win32-specific text drawing is isolated behind `Win32ComponentRenderer`.
- live runtime service/update status continues to refresh correctly.
- client-area dimensions continue to track resizing correctly.
- repaint behavior remains clean without ghosted text.

This confirms that the application view can be separated from Win32 drawing calls without changing the visible behavior on the target machine.

## 2026-09-15 — Phase 2 container/layout validation

The container and stack-layout tranche was rebuilt and run on the Pentium 4 target.

Validated commit:

```text
01892b6 Add container and stack layout framework
```

Validated behavior:

- Visual C++ 7.1 accepts the new `Container` and `StackPanel` classes.
- the status screen renders through the container child hierarchy.
- vertical stack layout reproduces the previous status presentation.
- live status updates continue to render correctly.
- resizing keeps the component stack centered.
- normal shutdown remains functional.

The visible output remained intentionally identical to the pre-container build, confirming that layout responsibility moved into the framework without changing application behavior.

## 2026-09-15 — Phase 2 interactive-control validation

The interactive framework tranche was rebuilt and exercised on the target Pentium 4.

Relevant commits:

```text
e574395 Add interactive framework controls and event dispatch
93b54f0 Fixed missing 'UIEvent.h' include directive
```

The first rebuild exposed one compile error: `Component.cpp` used `UIEvent` without including its definition. The missing `#include "UIEvent.h"` directive was added and pushed from the target machine. The corrected build then compiled, linked, and ran successfully.

Validated behavior:

- backend-neutral `Button` renders and reacts to pointer interaction,
- backend-neutral `TextInput` accepts focus and text entry,
- printable character input and Backspace operate,
- framework events travel from Win32 messages through `View` and `Container`,
- button callbacks update application state,
- style primitives render through the Win32 backend,
- the interactive controls remain responsive while runtime status continues updating,
- resize behavior remains functional,
- and normal shutdown remains clean.

This validates the core Phase 2 interaction path on VC7.1 and the Server 2003 target.

## 2026-09-15 — Messenger layout and double-buffer validation

The messenger-style shell was rebuilt and exercised on both the primary Windows Server 2003 environment and MiniXP on the Pentium 4.

Validated layout behavior:

- the reusable `MessageInputStrip` renders with an expanding text field and right-anchored Send button,
- Enter-to-submit and button-click submission both operate,
- submitted text updates the conversation area,
- the composer clears after submission,
- the diagnostics sidebar participates in the responsive layout,
- and normal shutdown remains functional.

MiniXP exposed a repaint defect that was not apparent in the primary Server 2003 environment: the entire client area visibly flashed whenever the periodic runtime-status repaint occurred. The host was clearing and repainting the complete client area directly to the visible window, allowing the intermediate cleared frame to become visible.

The corrective rendering commit is:

```text
dd880eb Double-buffer Win32 painting to prevent flicker
```

The Win32 host now:

- suppresses the separate `WM_ERASEBKGND` pass,
- renders the complete application view into a compatible memory DC/bitmap,
- copies the completed frame to the real window DC with one `BitBlt`,
- and retains a direct-render fallback if a back-buffer allocation fails.

The corrected build was revalidated successfully on both Windows Server 2003 and MiniXP. The periodic runtime tick repaint no longer produces visible flashing in either environment.

This remains an immediate anti-flicker correction rather than the final rendering architecture. The longer-term framework direction is explicit invalidation/dirty-region tracking so components can request repaint of only the area that changed instead of requiring periodic full-window redraws.

## 2026-09-15 — Text cursor navigation validation

The cursor-navigation tranche was rebuilt and exercised on the Pentium 4 target.

Validated commit:

```text
7273f18 Add cursor navigation to text input
```

Validated behavior:

- Left/Right arrow movement operates inside the focused text field,
- Home/End navigation operates,
- Delete removes the character at the caret,
- Backspace removes the character before the caret,
- typed characters insert at the current caret position,
- the caret renders at its actual cursor position,
- and Enter-to-submit continues to operate after in-place editing.

## 2026-09-15 — Selection, clipboard, and selectable-label validation

The selection/clipboard tranche and subsequent selectable-label tranche were exercised on the Pentium 4 target.

Relevant commits:

```text
b38835b Add text selection clipboard and MIME input foundation
3247a81 Add word navigation and selectable read-only labels
7321067 Add multi-click text selection
ad147f1 Add discontinuous text selection and paste modes
0cc832e Add subtractive text selection gestures
20e0092 Add cut modes and text undo redo
```

Validated behavior across Windows Server 2003 SP2 and MiniXP includes:

- mouse text selection inside framework text controls,
- selectable read-only conversation-label text,
- copying selected label text through the framework clipboard path,
- Ctrl+Arrow / Ctrl+Shift+Arrow word-boundary navigation,
- double-click word selection and triple-click line selection,
- discontinuous additive selection with preserved ranges,
- Ctrl+Alt drag/word subtraction from highlighted ranges,
- compact and preserved-spacing clipboard representations,
- compact and keep-formatting paste modes,
- compact and keep-formatting cut modes,
- bounded Undo/Redo history,
- restoration of text, caret, and discontinuous selection state,
- and successful external clipboard interoperability.

The final advanced text/editing state is tagged:

```text
v0.0.2
```

## 2026-09-15 — Append-only conversation history validation

The previous single-message conversation label has been replaced by an append-only `ConversationView`. Historical messages remain present instead of being overwritten. Individual message labels retain read-only text-selection behavior and the view can move through older/newer content when the visible message capacity is exceeded.

## Current post-v0.0.2 composer-toolbar target

The next tranche moves the bottom application surface from a single `MessageInputStrip` to a composed `MessageComposer` containing a toolbar and the existing input strip.

Target behaviors to validate on the Pentium 4:

- `[+]` opens the native Win32 multi-file picker,
- multiple selected files enter the composer attachment buffer,
- attachment count is shown in the toolbar,
- attachment-only submissions are permitted,
- submitted attachments are represented in conversation history,
- `[B]`, `[I]`, and `[U]` are toggleable toolbar-state controls,
- Ctrl+B, Ctrl+I, and Ctrl+U toggle the same states,
- the font-size combo box selects a composer font-size state and opens upward to remain usable near the bottom of the window,
- the list control is visibly disabled while the composer remains single-line,
- the classic emoticon button opens a dedicated `EmojiPanel` beneath it,
- classic aliases such as `:)`, `:D`, `;)`, `:P`, `:'(`, `:O`, `<3`, and `<:` can be inserted at the current text caret,
- toolbar popups render above the message input and consume pointer input before underlying text/conversation surfaces,
- existing text selection, clipboard, Undo/Redo, Enter-to-submit, resize, and double-buffer rendering behavior remains intact.

Formatting controls in this tranche intentionally establish composer state only. Applying mixed Bold/Italic/Underline/font-size runs to selected or newly typed text is reserved for the upcoming rich-text document/input model rather than being bolted onto the current single-style `TextInput` string.
