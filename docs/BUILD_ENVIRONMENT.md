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
- Visual C++ 7.1 / MSVC 7.1 (`_MSC_VER == 1310`)
- Visual Studio 6.0 / Visual C++ 6.0
- .NET Framework 2.0
- Windows DDK/WDK installations
- Git
- 7-Zip

The machine is already capable of compiling legacy native Windows projects.

The installed .NET Framework 2.0 runtime does **not** imply that the Visual C++ 2005
compiler is installed. Visual Studio .NET 2003 remains the SalixWeb32 build toolchain;
its native compiler is VC/MSVC 7.1.

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

### Winsock2 must precede `windows.h`

When a Win32 translation unit uses Winsock2 directly, include `<winsock2.h>` **before** `<windows.h>`:

```cpp
#include <winsock2.h>
#include <windows.h>
```

Older Windows SDK header stacks can otherwise allow the original Winsock header pulled in through `windows.h` to conflict with Winsock2 declarations. Keep Winsock headers confined to the platform implementation when possible so this ordering rule does not leak into framework/application headers.

The first remote-bridge transport follows this rule in `Win32HttpTransport.cpp` and links `ws2_32.lib` explicitly.

### VC7.1 `FD_SET` warning behavior

The VC7.1 Winsock macros can emit warning C4127 at warning level 4 even for valid
`FD_SET` usage. SalixWeb32 avoids globally disabling that warning. The current
`Win32HttpTransport` path populates the small Winsock `fd_set` structures directly
for its single-socket connect wait and tests membership explicitly.

Keep warning suppressions narrow when a legacy SDK macro is the source; do not weaken
the project's warning policy globally.

### Legacy ShellAPI header ordering

The installed VC7.1-era Platform SDK `ShellAPI.h` assumes base Win32 declarations are
already available. Including it before `windows.h` can produce a cascade beginning
with missing `HDROP`, `DECLARE_HANDLE`, `HWND`, `HINSTANCE`, and related types.

SalixWeb32 provides a deliberately small project-local `src/shellapi.h` compatibility
header for the currently required `ShellExecuteA` declaration. It includes
`windows.h` first and avoids importing the problematic old SDK header wholesale.

This is a target-compatibility shim, not a general replacement for ShellAPI.


## Python experiment (historical / separate project candidate)

A preliminary CPython 3.13.3 experiment established only the following facts on the
Server 2003 target:

- source tarball extraction succeeds,
- `PCbuild/` is present in the upstream source,
- the official modern 32-bit installer does not launch,
- the observed error was `not a valid Win32 application`.

Further modern-Python-on-NT5 work is not a SalixWeb32 build prerequisite and should be
treated as a separate porting project if pursued. SalixWeb32 may later consume a
validated runtime through an optional scripting/provider boundary.
