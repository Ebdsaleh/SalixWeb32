# Current Legacy Build Environment

## Target machine

```text
Windows Server 2003 Standard Edition SP2
x86
Pentium 4 class CPU
2 GB RAM
```

## Confirmed installed tooling

Confirmed on the target system:

- Visual Studio .NET 2003
- Visual Studio 6.0 / Visual C++ 6.0
- .NET Framework 2.0
- Windows DDK/WDK installations
- Git
- 7-Zip

The machine is already capable of compiling legacy native Windows projects.

## VC7.1 / legacy Platform SDK gotchas

SalixWeb32 is intentionally compiled with Visual C++ 7.1 / Visual Studio .NET 2003 on the real target machine. Some Win32 constants and declarations that are normal on newer SDKs may be absent or exposed differently by the older headers. Treat these as compatibility issues in the platform layer rather than reasons to raise the application's global Windows target unnecessarily.

### `WM_MOUSEWHEEL` may be missing

This has now occurred more than once during development. Code that handles mouse-wheel input may fail under the VC7.1-era headers with errors such as:

```text
error C2065: 'WM_MOUSEWHEEL': undeclared identifier
error C2051: case expression not constant
```

For code paths that only need the documented Win32 message value, use a narrow compatibility guard near the platform implementation:

```cpp
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif
```

Do **not** respond to this particular header omission by globally increasing `WINVER` or `_WIN32_WINNT` just to make the symbol appear. Raising those macros can expose newer declarations elsewhere and may accidentally weaken the NT5/legacy compatibility discipline. Prefer the smallest local compatibility definition when the runtime message/API itself is valid on the supported target.

When adding new Win32 input, shell, common-control, or window-message code, specifically check whether VC7.1's installed Platform SDK exposes every constant and declaration used by the implementation before considering the tranche target-validated.

### Common-controls include order

The VC7.1-era SDK can also be sensitive to header ordering around common controls. When using `<commctrl.h>`, ensure the normal Win32 declarations have already been introduced (for example through `windows.h` or the relevant SalixWeb32 Win32 host header) before including the common-controls header.

This previously affected the native tab implementation and should be treated as another known legacy-SDK compatibility check rather than as an application-architecture issue.

## Python experiment

CPython 3.13.3:

- source tarball extracted successfully,
- `PCbuild/` directory present,
- official 32-bit installer does not launch on Server 2003,
- observed error: `not a valid Win32 application`.

The next useful Python step is to capture the **first failure from the stock source build**, then decide whether build-system, compiler, PE-subsystem, CRT, or runtime API compatibility is the primary blocker.

Do not treat Python as a prerequisite for Phase 1 of SalixWeb32.
