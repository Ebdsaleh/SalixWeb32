# SalixWeb32

**SalixWeb32** is an experimental modular modern-web/application runtime for legacy and constrained Win32 systems, beginning with Windows Server 2003 SP2 x86 on Pentium 4 hardware.

The first proof-of-concept will be an interactive modern SaaS client, but the project is deliberately **not** architected around any one website or service. The long-term goal is a reusable runtime in which web-platform capabilities can be supplied by interchangeable native, ported, translated, or remote backends.

## Core idea

SalixWeb32 separates:

- the native application shell,
- reusable application/runtime infrastructure,
- UI framework concepts,
- Win32 presentation,
- and the modern web platform.

The application should not depend directly on LibreWolf, Firefox, Gecko, Chromium, Python, or any one browser engine.

```text
Application
    |
    +-- app/
    +-- runtime/
    +-- framework/
    +-- engine/win32/
    |
    +-- web/
         +-- network/
         +-- security/
         +-- document/
         +-- scripting/
         +-- style/
         +-- layout/
         +-- graphics/
         +-- storage/
         +-- backends/
```

A future `WebPlatformBackend` boundary will allow the application to use different implementations such as:

- native Salix modules,
- ported Gecko-derived components,
- a translation backend,
- a remote backend,
- or mixed implementations.

## Initial target

- CPU: Pentium 4 class, SSE2-era hardware
- RAM: 2 GB
- OS: Windows Server 2003 Standard / SP2, x86
- Toolchain currently available: Visual Studio .NET 2003 / Visual C++ 7.1, Visual Studio 6, multiple Windows DDK/WDK environments
- Runtime goal: native Win32 first
- Python: optional future scripting/plugin capability, never a mandatory runtime dependency

## Current state

Phase 1 is complete and validated on the target Pentium 4 under Windows Server 2003 SP2 x86. The same executable has also passed a MiniXP smoke test.

Phase 2 framework/presentation separation is now largely target-validated. The application uses backend-neutral views, components, containers, stack layout, labels, buttons, text input, event dispatch, and style primitives, with Win32 drawing isolated behind `Win32ComponentRenderer`.

The current layout pass is moving the shell toward a late-2000s messenger-style composition: a header, conversation surface, diagnostics sidebar, and reusable `MessageInputStrip` with an expanding text field, right-anchored Send button, and configurable `submit_on_enter` behavior.

See:

- `ROADMAP.md`
- `ARCHITECTURE.md`
- `docs/CODING_STYLE.md`
- `docs/VALIDATION.md`
- `docs/WEB_PLATFORM_CAPABILITIES.md`
- `docs/BUILD_ENVIRONMENT.md`
- `docs/DECISIONS.md`

## License

Not selected yet. Do not add a license until the project owner chooses one.
