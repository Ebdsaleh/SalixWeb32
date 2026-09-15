# SalixWeb32 Roadmap

This roadmap intentionally separates the native application/platform skeleton from the much harder modern-web compatibility work.

Status legend:

```text
[ ] not started
[~] experimental / active
[x] complete
```

## Phase 0 — Repository and architecture baseline

- [x] Select working name: `SalixWeb32`
- [x] Define modular architecture
- [x] Define web platform as backend-independent
- [x] Define native/Gecko/translator/remote backend families
- [x] Record hardware/toolchain constraints
- [x] Create public architecture and roadmap documentation
- [ ] Create initial Git repository
- [ ] Select license
- [ ] Establish coding conventions
- [ ] Add first tagged baseline once skeleton builds

**Exit criterion:** repository exists with documentation and agreed source layout.

## Phase 1 — Native Win32 application skeleton

Goal: prove the Salix architecture without any browser engine.

- [ ] Create VS.NET 2003-compatible solution/project
- [ ] Native `WinMain`
- [ ] `ApplicationRuntime`
- [ ] service lifecycle (`start/update/stop`)
- [ ] `ServiceRegistry`
- [ ] diagnostics/logging foundation
- [ ] `Win32ApplicationHost`
- [ ] main window creation
- [ ] resize handling
- [ ] clean shutdown
- [ ] basic status reporting

**Exit criterion:** a native x86 executable runs correctly on Server 2003 SP2 and cleanly opens/closes a window.

## Phase 2 — Framework / presentation separation

Goal: reproduce the useful architectural separation demonstrated by SalixTorrent.

- [ ] semantic `Component` base
- [ ] container component
- [ ] label
- [ ] button
- [ ] text input
- [ ] basic row/column layout concepts
- [ ] event dispatch
- [ ] styling primitives
- [ ] `ComponentRenderer` contract
- [ ] Win32 component renderer
- [ ] application view independent of Win32 calls

**Exit criterion:** application UI is described through framework components and rendered by the Win32 backend.

## Phase 3 — WebView and backend contract

Goal: web support has a stable slot before any engine is chosen.

- [ ] `WebView` semantic component
- [ ] `WebPlatformBackend` contract
- [ ] backend lifecycle
- [ ] navigation request model
- [ ] document/render surface contract
- [ ] input/event bridge
- [ ] placeholder backend
- [ ] diagnostics capability reporting
- [ ] backend selection mechanism

Initial backend families:

```text
native
gecko
translator
remote
```

**Exit criterion:** shell displays a WebView supplied by a dummy backend without product code knowing the concrete backend type.

## Phase 4 — Web foundation primitives

Goal: implement or port reusable pieces that every web backend needs.

- [ ] URL parsing / canonicalization
- [ ] MIME handling
- [ ] Unicode/text encoding abstraction
- [ ] stream/buffer abstraction
- [ ] network request/response model
- [ ] cookie model
- [ ] cache abstraction
- [ ] certificate/trust abstraction
- [ ] security origin model
- [ ] content decoding/compression investigation

Do not implement cryptography from scratch.

**Exit criterion:** clean interfaces exist for network/security/document layers even if some providers remain stubs.

## Phase 5 — Network + HTTPS viability

Goal: retrieve modern HTTPS content safely enough for experimentation.

- [ ] identify portable TLS candidates
- [ ] verify compiler/NT 5.2 compatibility
- [ ] DNS/socket layer
- [ ] HTTP/1.1 client
- [ ] redirects
- [ ] headers
- [ ] TLS 1.2 minimum
- [ ] certificate validation
- [ ] proxy support
- [ ] request logging/endpoint visibility
- [ ] investigate HTTP/2 need
- [ ] defer HTTP/3 unless target requires it

**Exit criterion:** P4 can retrieve selected modern HTTPS resources through the Salix network layer.

## Phase 6 — Basic document engine

Goal: render static web documents without JavaScript.

- [ ] HTML tokenizer/parser strategy
- [ ] `WebDocument`
- [ ] `WebNode`
- [ ] attributes/text/tree mutation
- [ ] basic CSS parser
- [ ] selector matching
- [ ] cascade/inheritance
- [ ] computed style representation
- [ ] block/inline layout
- [ ] scrolling
- [ ] text rendering
- [ ] links
- [ ] images
- [ ] basic forms

**Exit criterion:** useful static HTML/CSS pages render inside WebView.

## Phase 7 — Modern layout and graphics

- [ ] positioning
- [ ] overflow/clipping
- [ ] flexbox
- [ ] CSS grid
- [ ] transforms
- [ ] opacity/compositing
- [ ] SVG investigation
- [ ] Canvas investigation
- [ ] font loading
- [ ] text shaping/bidi strategy
- [ ] image format support

**Exit criterion:** a substantial class of modern non-SaaS pages render acceptably.

## Phase 8 — JavaScript engine integration

Do not write a JS engine from scratch.

Candidate investigation track:

- [ ] SpiderMonkey feasibility
- [ ] QuickJS feasibility
- [ ] other lightweight embeddable engines
- [ ] memory/CPU benchmarks on P4
- [ ] ES-version compatibility
- [ ] host API bridge design

Then implement:

- [ ] script execution
- [ ] DOM bindings
- [ ] events
- [ ] timers
- [ ] promises/microtasks
- [ ] modules where required

**Exit criterion:** JavaScript can manipulate the Salix document model.

## Phase 9 — Interactive Web APIs

Implement only what real target applications require.

- [ ] `fetch`
- [ ] XHR if needed
- [ ] forms
- [ ] file input
- [ ] history/navigation
- [ ] clipboard subset
- [ ] localStorage
- [ ] sessionStorage
- [ ] WebSocket
- [ ] Server-Sent Events
- [ ] Streams subset
- [ ] IndexedDB investigation
- [ ] Web Components / Shadow DOM
- [ ] workers
- [ ] service workers
- [ ] WebAssembly

**Exit criterion:** selected modern SaaS applications reach functional login/session/UI states.

## Phase 10 — First SaaS proof: interactive chat client

Goal: solve the original practical problem.

Possible approaches may coexist:

```text
native site-specific adapter
local web backend
translation backend
remote backend
```

- [ ] conversation display
- [ ] text input
- [ ] attachments
- [ ] file transfer
- [ ] copy/paste build logs
- [ ] session persistence
- [ ] diagnostics panel
- [ ] privacy/network endpoint visibility

**Exit criterion:** modern interactive SaaS communication is usable directly from the P4 without an external transfer workflow.

## Phase 11 — Security hardening

This phase starts conceptually much earlier but becomes a dedicated milestone here.

- [ ] same-origin enforcement
- [ ] CORS
- [ ] CSP
- [ ] secure-context rules
- [ ] cookie isolation
- [ ] permissions model
- [ ] sandboxing strategy
- [ ] renderer isolation strategy
- [ ] malicious-content test corpus
- [ ] certificate failure UX
- [ ] telemetry audit
- [ ] background endpoint audit

**Exit criterion:** security model is explicit, testable, and not merely inherited accidentally.

## Phase 12 — Optional Python host

Python is an optional capability, not a dependency.

Parallel investigation:

- [~] CPython 3.13.3 source extracted on Server 2003
- [x] official x86 installer tested
- [x] installer rejected as `not a valid Win32 application`
- [ ] establish first stock-source build failure
- [ ] identify viable compiler/toolchain
- [ ] port/runtime compatibility investigation
- [ ] embed successful interpreter behind scripting interface
- [ ] expose selected Salix services to Python
- [ ] investigate reuse/migration of SalixTorrent framework concepts

**Exit criterion:** Python scripts/plugins can run if the module is present; SalixWeb32 still works if it is absent.

## Phase 13 — Portability / productization

- [ ] Windows XP x86 testing
- [ ] Windows Server 2003 variants
- [ ] Windows 2000 feasibility
- [ ] newer Win32 testing
- [ ] installer/portable package
- [ ] crash diagnostics
- [ ] plugin/module packaging
- [ ] developer SDK
- [ ] documentation
- [ ] examples
- [ ] performance profiling on actual P4
- [ ] release/versioning policy

# Guiding priority

At every phase prefer:

```text
small validated module
        over
massive speculative port
```

and:

```text
clean capability boundary
        over
hard-coded dependency on one browser engine
```
