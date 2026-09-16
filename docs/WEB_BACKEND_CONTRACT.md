# WebView and Web Backend Contract

This document records the first Phase 3 SalixWeb32 web-platform boundary.

## Purpose

The goal of this tranche is **not** to browse the modern web yet. It establishes the stable architectural slot through which a future native, Gecko, translator, or remote implementation can supply web capability without the application shell depending on that concrete implementation.

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
        +-- PlaceholderWebBackend   (Phase 3 validation backend)
        +-- NativeBackend           (future)
        +-- GeckoBackend            (future)
        +-- TranslatorBackend       (future)
        `-- RemoteBackend           (transport work now active)
```

The placeholder backend performs **no network access**. It exists so the component, lifecycle, navigation, surface, input, diagnostics, and backend-selection boundaries can be compiled and exercised on the Pentium 4 before a real web engine is chosen.

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

`placeholder` is a development/validation family only. The remaining families match the long-term architecture described in `ARCHITECTURE.md`.

## WebPlatformHost

`WebPlatformHost` owns the **selected backend boundary**, not the concrete backend object itself.

The composition root supplies a backend before runtime initialization:

```text
Concrete backend
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

The shell currently submits an initial target of:

```text
https://www.chatgpt.com/
```

The placeholder accepts the request only as state. **It does not contact the address.** The point is to prove that another backend can receive the same application request without changing the shell.

## WebSurfaceSnapshot

The first surface contract is intentionally modest:

```text
title
address
status
content
```

This is a Phase 3 diagnostic/presentation surface, not an attempt to define the future DOM or full rendering engine.

A future backend may expose a document tree, raster surface, retained display list, native child surface, or another representation behind an extended contract. The application must not assume that this first textual snapshot is the final browser rendering architecture.

## WebView

`framework::WebView` is a semantic framework component built from existing framework primitives.

It displays:

- backend identity and family,
- lifecycle state,
- requested address,
- explicit capability flags,
- backend surface status/content.

It binds only to `WebPlatformHost`; it does not know a concrete backend exists.

The application shell contains:

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

The placeholder reports only the contract capabilities it genuinely implements:

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

## Pentium 4 validation result

The Phase 3 foundation was rebuilt and exercised on the real Pentium 4 under Windows Server 2003 SP2 with Visual C++ 7.1 on September 16, 2026.

The target pass was reported **green with no build errors or warnings**.

Observed in the running application:

- `Conversation`, `Web`, and `Runtime` workspace tabs are present,
- `Web` identifies `Placeholder Web Backend`, family `placeholder`, lifecycle `initialized`,
- the requested address is `https://www.chatgpt.com/`,
- capability reporting correctly distinguishes supported contract features from unavailable network/HTML/JS/WebSocket/upload features,
- the placeholder explicitly states that no network request is being made,
- forwarded input events increase while interacting with the Web workspace,
- the application remains responsive and the new workspace renders correctly on the legacy target.

This satisfies the Phase 3 Server 2003 exit criterion: the shell displays a `WebView` supplied by a dummy backend without product code knowing the concrete backend type.

MiniXP remains a separate smoke target and is not implied by this Server 2003 result.

The next transport work is documented in `docs/REMOTE_BRIDGE.md`.

## Phase 3 regression checklist

Future backend work should preserve these validated behaviors:

1. VC7.1 continues to compile/link the generic web-platform boundary.
2. SalixWeb32 starts even when no real network backend is selected.
3. Workspace switching preserves Conversation native-control lifecycle.
4. Web backend identity/family/lifecycle remain visible.
5. Capability reporting remains honest and backend-driven.
6. Input forwarding remains backend-neutral.
7. Conversation composition, attachments, Preview/Open, and scrolling do not regress.
8. Backend/runtime shutdown remains clean.
9. MiniXP smoke coverage is recorded separately when explicitly performed.
