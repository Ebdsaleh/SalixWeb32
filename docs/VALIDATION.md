# SalixWeb32 Target Validation

This document records validated legacy-target baselines and the major defects found while validating them. Active work after `v0.0.2` is tracked in `docs/POST_V002_VALIDATION.md` so experimental composer/conversation tranches can advance without rewriting the tagged-baseline history.

## Target environments

Primary target:

```text
CPU:        Pentium 4 class
Memory:     2 GB
OS:         Windows Server 2003 SP2 x86
Compiler:   Visual C++ 7.1 / Visual Studio .NET 2003
Executable: SalixWeb32.exe
```

Secondary smoke target:

```text
MiniXP on the same Pentium 4 hardware
```

MiniXP results are smoke tests and do not replace testing on a clean retail Windows XP installation.

## Phase 1 native skeleton baseline

Validated commit:

```text
4628025 Add Phase 1 native Win32 runtime skeleton
```

Validated on Windows Server 2003:

- VS.NET 2003 solution opens successfully,
- Debug configuration compiles and links with VC7.1,
- native application launches from Visual Studio and Command Prompt,
- `ApplicationRuntime` initializes,
- `Win32ApplicationHost` creates the application window,
- message loop remains responsive,
- status text renders,
- normal close exits cleanly.

The same executable also launched successfully under MiniXP.

## Phase 1 hardening

Relevant commits:

```text
5547277 Harden Phase 1 runtime lifecycle and resize handling
0fd362e Fix Win32 status repaint ghosting
```

Validated behavior:

- `RuntimeStatusService` starts through `ServiceRegistry`,
- runtime ticks advance continuously,
- service count reports correctly,
- client-area dimensions update during resize,
- normal shutdown remains clean.

Target validation exposed repaint ghosting because live status text was redrawn over stale pixels with a transparent text background. Clearing the client area inside the paint path corrected the defect. The corrected build was revalidated with repeated resizing and sustained runtime updates.

The validated Phase 1 baseline is tagged:

```text
v0.0.1
```

## Phase 2 presentation separation

Validated commit:

```text
c3a3406 Begin Phase 2 framework presentation separation
```

Validated on the target:

- framework `Label` components represent status text,
- `StatusView` renders through the backend-neutral `View` contract,
- Win32-specific drawing remains behind `Win32ComponentRenderer`,
- live status and resize behavior remain correct,
- repaint ghosting does not regress.

## Phase 2 container/layout

Validated commit:

```text
01892b6 Add container and stack layout framework
```

Validated behavior:

- VC7.1 accepts `Container` and `StackPanel`,
- status presentation renders through the child hierarchy,
- vertical stack layout reproduces the previous presentation,
- live updates, resize, and shutdown remain functional.

## Phase 2 interactive controls

Relevant commits:

```text
e574395 Add interactive framework controls and event dispatch
93b54f0 Fixed missing 'UIEvent.h' include directive
```

The first rebuild exposed a compile error because `Component.cpp` used `UIEvent` without including its definition. The missing include was added from the target machine. The corrected build compiled, linked, and ran successfully.

Validated behavior:

- backend-neutral `Button` pointer interaction,
- backend-neutral `TextInput` focus/text entry,
- printable character input and Backspace,
- Win32-to-framework event dispatch,
- callbacks and styling primitives,
- continued runtime updates and clean resize/shutdown behavior.

## Messenger layout and double buffering

The messenger shell and `MessageInputStrip` were exercised on both Windows Server 2003 and MiniXP.

Validated behavior included:

- expanding text field with right-anchored Send button,
- Enter-to-submit and button-click submission,
- composer clear after send,
- responsive diagnostics sidebar.

MiniXP exposed full-window flashing during periodic runtime repaints. The fix:

```text
dd880eb Double-buffer Win32 painting to prevent flicker
```

changed the Win32 host to suppress the separate erase pass, render the complete application into a compatible memory DC/bitmap, and `BitBlt` the completed frame to the real DC. The corrected build was revalidated successfully on both target environments with no observed periodic flashing.

Dirty-region/component invalidation remains a later rendering improvement.

## Text cursor navigation

Validated commit:

```text
7273f18 Add cursor navigation to text input
```

Validated behavior:

- Left/Right movement,
- Home/End,
- Delete and Backspace,
- insertion at the caret,
- visual caret at the actual cursor position,
- Enter-to-submit after in-place editing.

## Selection, clipboard, and advanced editing baseline

Relevant commits:

```text
b38835b Add text selection clipboard and MIME input foundation
3247a81 Add word navigation and selectable read-only labels
7321067 Add multi-click text selection
ad147f1 Add discontinuous text selection and paste modes
0cc832e Add subtractive text selection gestures
20e0092 Add cut modes and text undo redo
```

Validated across Windows Server 2003 SP2 and MiniXP:

- mouse selection in framework text controls,
- selectable read-only conversation text,
- clipboard copy from read-only labels,
- Ctrl+Arrow / Ctrl+Shift+Arrow word navigation,
- double-click word and triple-click line selection,
- discontinuous/additive selections,
- Ctrl+Alt subtractive selection workflows,
- compact and preserved-spacing clipboard representations,
- compact and keep-formatting paste modes,
- compact and keep-formatting cut modes,
- bounded Undo/Redo,
- restoration of text/caret/discontinuous selection state,
- external clipboard interoperability.

Interaction detail recorded during validation: a preserved highlighted range can remain selected while Ctrl+click relocates the caret. Additional regions can be added while retaining the existing highlight through the established additive selection gesture (including Shift+drag where appropriate). This behavior is intentional and is part of the documented selection model.

The final advanced editing baseline is tagged:

```text
v0.0.2
```

## Append-only conversation history

The previous single-message conversation label was replaced by an append-only `ConversationView`. Historical messages remain present and independently selectable instead of being overwritten.

This became the foundation for later native conversation scrolling, Markdown presentation, wrapping, and block-composed messages.

## Post-v0.0.2 validation

Post-baseline work now includes:

- toolbar/composer formatting,
- graphical classic emoticons,
- multiline/list editing,
- code composer mode,
- Markdown presentation,
- native composer/conversation scrollbars,
- mouse-wheel input,
- wrapped pixel-scrolled conversation history,
- code-vs-normal semantic presentation,
- the current dedicated code-block container work.

These newer tranches do **not** inherit the `v0.0.2` validation status automatically.

See:

```text
docs/POST_V002_VALIDATION.md
```

for the current Server 2003 observations, compatibility fixes, remaining MiniXP coverage, and the target checklist for the active block-composed code presentation tranche.
