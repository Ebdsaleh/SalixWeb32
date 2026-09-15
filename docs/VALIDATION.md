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

## Current hardening validation

The next Phase 1 tranche adds:

- a concrete runtime service exercising `start/update/stop`,
- duplicate-service protection in `ServiceRegistry`,
- visible runtime service/update status,
- explicit client-area resize tracking,
- repaint-on-resize behavior,
- additional shutdown diagnostic events.

These items remain pending target-hardware revalidation until the updated executable is rebuilt and exercised on the Pentium 4.
