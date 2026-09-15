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

## Python experiment

CPython 3.13.3:

- source tarball extracted successfully,
- `PCbuild/` directory present,
- official 32-bit installer does not launch on Server 2003,
- observed error: `not a valid Win32 application`.

The next useful Python step is to capture the **first failure from the stock source build**, then decide whether build-system, compiler, PE-subsystem, CRT, or runtime API compatibility is the primary blocker.

Do not treat Python as a prerequisite for Phase 1 of SalixWeb32.
