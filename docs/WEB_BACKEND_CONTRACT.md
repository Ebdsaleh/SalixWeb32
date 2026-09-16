# WebView and Web Backend Contract

This document records the first Phase 3 SalixWeb32 web-platform boundary.

## Purpose

The goal of this tranche is **not** to browse the modern web yet. It is to establish the stable architectural slot through which a future native, Gecko, translator, or remote implementation can supply web capability without the application shell depending on that concrete implementation.

The contract is deliberately useful before networking exists:

```text
Application shell
        |
        v
framework::WebView
        |
        v
web::platform::WebPlatformHost
        |
        v
web::platform::WebPlatformBackend
        |
        +-- PlaceholderWebBackend   (current validation backend)
        +-- NativeBackend           (future)
        +-- GeckoBackend            (future)
        +-- TranslatorBackend       (future)
        `-- RemoteBackend           (future)
```

The current placeholder backend performs **no network access**. It exists so the component, lifecycle, navigation, surface, input, diagnostics, and backend-selection boundaries can be compiled and exercised on the Pentium 4 before a real web engine is chosen.

## WebPlatformBackend

`WebPlatformBackend` is the interchangeable provider contract.

The first contract covers:

```text
identity
    backend name
    backend family

lifecycle
    initialize
    update
    shutdown
    initialized state

capabilities
    explicit feature reporting

navigation
    backend-neutral WebNavigationRequest

surface
    backend-neutral WebSurfaceSnapshot

input
    backend-neutral WebInputEvent
```

Application code must not include or branch on a concrete browser-engine type.

## Backend families

The initial family model is:

```text
placeholder
native
gecko
translator
remote
```

`placeholder` is a development/validation family only. The remaining families match the long-term architecture already described in `ARCHITECTURE.md`.

## WebPlatformHost

`WebPlatformHost` owns the **selected backend boundary**, not the concrete backend object itself.

The composition root supplies a backend before runtime initialization:

```text
PlaceholderWebBackend
        |
        v
WebPlatformHost::set_backend(...)
        |
        v
ApplicationRuntime lifecycle
```

Backend switching is intentionally rejected while the host is initialized in this first pass. This prevents a half-shut-down engine from being replaced underneath an active `WebView`. A later hot-switch design can add an explicit stop/swap/start transaction if it becomes useful.

The host exposes only generic operations to consumers:

```text
navigate
get surface snapshot
forward input
query capabilities
query family/name/state
```

## Runtime ownership

`ApplicationRuntime` accepts an optional `WebPlatformHost`.

Startup order is:

```text
register core runtime services
initialize selected web backend
start runtime services
```

Update order is:

```text
update web backend
update runtime services
```

Shutdown order is:

```text
stop runtime services
shutdown web backend
```

This keeps the web platform optional. SalixWeb32 can still initialize without a selected backend.

## WebNavigationRequest

Navigation is represented semantically rather than as a browser-specific function call.

The first request model contains:

```text
URL
kind
    typed
    link
    reload
replace-history flag
```

The current shell submits an initial target of:

```text
https://www.chatgpt.com/
```

The placeholder accepts the request only as state. **It does not contact the address.** The point is to prove that a real backend can later receive the same application request without changing the shell.

## WebSurfaceSnapshot

The first surface contract is intentionally modest:

```text
title
address
status
content
```

This is a Phase 3 diagnostic/presentation surface, not an attempt to define the future DOM or full rendering engine.

A future backend may eventually expose a document tree, raster surface, retained display list, native child surface, or another representation behind an extended contract. The application must not assume that this first textual snapshot is the final browser rendering architecture.

## WebView

`framework::WebView` is a semantic framework component built from existing framework primitives.

It displays:

- backend identity and family,
- lifecycle state,
- requested address,
- explicit capability flags,
- backend surface status/content.

It binds only to `WebPlatformHost`; it does not know `PlaceholderWebBackend` exists.

The first application shell adds a third workspace tab:

```text
Conversation | Web | Runtime
```

Conversation-native child controls are detached while `Web` is active in exactly the same way they are detached for `Runtime`, preserving the existing native-control lifecycle discipline.

## Input bridge

`WebView` maps the framework's existing `UIEvent` representation into a web-platform `WebInputEvent` for:

```text
mouse move
mouse down/up
mouse wheel
key down/up
character input
```

The placeholder only counts these events. No browser semantics are inferred yet. This establishes direction of travel without prematurely designing DOM event behavior around a fake engine.

## Capability discovery

Backends report support explicitly through `WebBackendCapabilities`.

The first fields are:

```text
navigation
surface snapshot
pointer input
keyboard input
network
HTML
CSS
JavaScript
WebSocket
file upload
```

The placeholder reports only the contract capabilities it genuinely implements. In particular it reports:

```text
navigation       yes
surface snapshot yes
pointer input    yes
keyboard input   yes
network          no
HTML             no
CSS              no
JavaScript       no
WebSocket        no
file upload      no
```

The `Runtime` tab and the `Web` tab both expose capability information so missing features are visible rather than silently implied.

## Architectural rule

The important rule for every later backend tranche is:

> Add capability behind `WebPlatformBackend`; do not make the application shell depend on the implementation that happens to provide it.

A native implementation may compose Salix network/document/layout subsystems. A Gecko implementation may delegate those jobs internally. A translator or remote backend may receive a representation produced elsewhere. Those differences belong below the backend boundary.

## Target validation checklist

This Phase 3 foundation is pending VC7.1/target validation until the real Pentium 4 rebuild is green.

1. Close/reopen Visual Studio .NET 2003 because the `.vcproj` gains new translation units.
2. Clean and rebuild the Debug Win32 solution with VC7.1.
3. Confirm SalixWeb32 still launches on Windows Server 2003 SP2.
4. Confirm the workspace now contains `Conversation`, `Web`, and `Runtime` tabs.
5. Switch repeatedly among all three tabs and confirm native Conversation controls disappear/reappear correctly.
6. Open the `Web` tab and confirm it identifies `Placeholder Web Backend` and family `placeholder`.
7. Confirm the Web tab displays `https://www.chatgpt.com/` as the navigation target while explicitly stating that no network request is performed.
8. Confirm capability reporting shows navigation/surface/input available and network/HTML/JavaScript/upload unavailable.
9. Move/click/type while the Web tab is active and confirm the forwarded-input counter changes without crashing.
10. Open the `Runtime` tab and confirm backend identity, family, lifecycle, and concise capability information are visible there too.
11. Confirm Conversation composition, attachments, image Preview/Open, and existing scrolling still work after visiting the Web tab.
12. Close the application normally and confirm backend/runtime shutdown is clean.
13. After Server 2003 passes, repeat the workspace/lifecycle smoke test under MiniXP.

Only after that target pass should this Phase 3 foundation be marked validated.
