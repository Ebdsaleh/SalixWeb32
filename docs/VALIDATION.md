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

## Current Phase 2 text-editing validation target

The current text-input tranche adds:

- a backend-neutral key-code vocabulary in `UIEvent`,
- Win32 virtual-key translation inside the host rather than leaking `VK_*` constants into framework controls,
- an explicit cursor position in `TextInput`,
- Left/Right arrow cursor movement,
- Home/End navigation,
- Delete-at-cursor behavior,
- Backspace-before-cursor behavior,
- character insertion at the current cursor position,
- and caret rendering at the current cursor position rather than always at the end of the text.

These items remain pending target-hardware validation until the updated project is rebuilt and exercised on the Pentium 4.
