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

## Current Phase 2 validation target

The current tranche adds:

- a backend-neutral `Container`,
- non-owning child registration/removal,
- ordered child rendering,
- `StackPanel` vertical and horizontal layout modes,
- automatic centered row/column arrangement,
- migration of the status screen from manual label positioning to the stack layout.

These items remain pending target-hardware validation until the updated project is rebuilt and exercised on the Pentium 4.
