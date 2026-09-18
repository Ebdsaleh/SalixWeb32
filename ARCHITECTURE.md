# SalixWeb32 Architecture

## Architectural goals

SalixWeb32 must:

1. run natively on constrained Win32 systems,
2. preserve separation between application, framework, runtime, presentation, and web platform,
3. allow web capabilities to be supplied by interchangeable modules/backends,
4. avoid making Python, Gecko, Chromium, or any vendor service mandatory,
5. allow gradual progress from a native shell to increasingly capable modern-web support,
6. remain useful even if full general-purpose browser compatibility is never achieved.

## Layer model

```text
+--------------------------------------------------+
|                   Application                    |
| product logic, chat/session logic, app views     |
+--------------------------------------------------+
|                    Framework                     |
| semantic components, layout, styling, events     |
+--------------------------------------------------+
|                     Runtime                      |
| lifecycle, services, registries, diagnostics     |
+--------------------------------------------------+
|                     Engine                       |
| Win32 application host + presentation backend    |
+--------------------------------------------------+
|                  Web Platform                    |
| network/document/script/layout/security/etc.     |
+--------------------------------------------------+
|                    Platform                      |
| Win32, sockets, filesystem, timers, graphics     |
+--------------------------------------------------+
```

Dependencies should generally flow downward.

## Web backend boundary

`WebView` is a framework-level semantic component.

It delegates through a selected web-platform provider rather than depending on a concrete browser engine:

```text
WebView
  |
  v
WebPlatformHost
  |
  v
WebPlatformBackend
  |
  +-- PlaceholderWebBackend
  +-- NativeBackend
  +-- GeckoBackend
  +-- TranslatorBackend
  `-- RemoteBridgeWebBackend
```

`WebPlatformHost` is the backend-selection and lifecycle boundary. It can be given one concrete backend by the composition root before runtime initialization. Consumers see only generic navigation, surface, input, capability, and identity operations.

The application must not branch on Gecko-specific types, backend-specific DOM objects, remote-bridge packet structures, or any other concrete provider detail.

The first Phase 3 implementation used `PlaceholderWebBackend` to prove the entire dependency direction on VC7.1 and the real target before a real network provider was selected. That contract has now passed the primary Server 2003 target build/runtime validation.

See `docs/WEB_BACKEND_CONTRACT.md` for the concrete Phase 3 contract and validation result.

## Web platform lifecycle relationship

`ApplicationRuntime` may be supplied an optional `WebPlatformHost`.

The lifecycle is deliberately explicit:

```text
startup
    register runtime services
    initialize selected web backend
    start runtime services

update
    update selected web backend
    update runtime services

shutdown
    stop runtime services
    shutdown selected web backend
```

This keeps the web platform optional. The native runtime remains capable of starting with no selected backend.

Backend replacement is not allowed while the host is initialized in the first implementation. A future hot-swap feature must use an explicit stop/swap/start transaction rather than changing a live provider underneath `WebView`.

## Conversation service boundary

The native Conversation workspace is separated from provider/runtime implementation by a
second explicit host/backend boundary:

```text
MessageComposer
    |
    v
MessageDraft
    |
    v
ConversationRequest
    |
    v
ConversationServiceHost
    |
    v
ConversationServiceBackend
    |
    +-- PlaceholderConversationBackend
    +-- RemoteConversationBackend
    `-- future API/web-session/local backends

semantic events
    |
    v
StatusView / application thread
    |
    v
ConversationView
```

The generic event vocabulary currently includes:

```text
request_started
message_started
text_delta
message_completed
request_failed
```

The service backend returns semantic content rather than remote UI state. Provider DOM
objects, browser automation objects, Python implementation objects, and provider-specific
response types do not cross this boundary.

The first `PlaceholderConversationBackend` performs no network access and has validated
the request/event lifecycle and native streaming presentation on the P4.

`RemoteConversationBackend` reuses the existing backend-neutral network contracts but
owns a **separate Win32 request executor/HTTP transport instance** from Browser Probe.
This avoids Browser and Conversation single-flight/lifecycle contention while allowing
both to reach the same companion host/port. The initial remote endpoint is probe-only: it transmits the Salix request ID plus fixed
zero-forwarding flags, not the draft body or attachment paths.

Before accepting a Conversation probe, `RemoteConversationBackend` asynchronously checks
`GET /v1/health`. The companion must advertise both
`conversation_probe=enabled` and the exact
`conversation_protocol=SALIX-CONVERSATION/1`. Backend readiness is therefore a
negotiated capability rather than an assumption based solely on TCP reachability or the
fact that Browser Probe works.

`ApplicationRuntime` explicitly initializes, updates, and shuts down the optional
`ConversationServiceHost` alongside the optional `WebPlatformHost`. Event consumption
and `ConversationView` mutation remain on the application thread.

See `docs/CONVERSATION_SERVICE_CONTRACT.md`.

## Persistent file-location boundary

SalixWeb32 does not treat the process current working directory as durable application
state. Startup resolves immutable application roots before any file dialog can run.

```text
                    STANDARD                         PORTABLE (--portable)

executable_root     executable directory             executable directory

user_data_root      %APPDATA%\SalixWeb32            executable directory

settings.ini        user_data_root\settings.ini      user_data_root\settings.ini

Diagnostics         user_data_root\Diagnostics       user_data_root\Diagnostics

Attachment browser  independent remembered directory independent remembered directory
```

The launch directory is still captured once for development/local-config discovery and
as the first-run attachment-browser location. It is not a persistent application storage
root.

The attachment picker uses `OFN_NOCHANGEDIR`. Diagnostic screenshot/report capture and
Browser Diagnostic Report export receive their destination explicitly rather than
calling `GetCurrentDirectoryA`.

`--portable` is a process-startup mode, not a mutable Settings checkbox. Standard mode
never silently falls back to the executable or launch directory for writable persistent
state; portable mode opts into executable-directory storage deliberately.

User-facing paths are kept separate from `salixweb32.local.ini`, which remains the
machine/development bridge configuration layer.

This establishes the broader rule that unrelated UI navigation state must never silently
redirect another subsystem's persistent output. See `docs/FILE_LOCATIONS.md`.

## Navigation, surface, and input contracts

The first Phase 3 contracts are intentionally small and backend-neutral.

`WebNavigationRequest` contains a URL, navigation kind, and replace-history intent. It does not expose browser-engine navigation objects.

`WebSurfaceSnapshot` currently contains:

```text
title
address
status
content
```

This is a diagnostic Phase 3 surface used to validate the architecture. It is **not** the long-term DOM or rendering model. Future backends may extend the boundary with a document tree, raster surface, retained display list, native child surface, or another suitable representation.

`WebInputEvent` receives normalized pointer, wheel, key, and character input from `WebView`. The placeholder backend only counts these events; DOM/browser event semantics remain future web-platform work.

## Web platform subsystems

```text
web/
+-- platform/   shared contracts and capability discovery
+-- network/    DNS, sockets, HTTP, cache, proxy
+-- security/   TLS integration, origins, CORS, CSP, permissions
+-- document/   HTML parsing, DOM, document model
+-- scripting/  JS engine integration and host bindings
+-- style/      CSS parser/cascade/computed style
+-- layout/     boxes, flex/grid, geometry, scrolling
+-- graphics/   paint, text, images, SVG/canvas/composition
+-- storage/    cookies, local/session storage, IndexedDB
`-- backends/   concrete compositions of the above
```

No rule says every backend must use every native Salix subsystem.

A Gecko backend may internally delegate many of them to Gecko. A native backend may compose Salix implementations. A translator may receive an already transformed document representation. A remote backend may consume a surface or semantic representation generated on another machine.

## Remote bridge transport relationship

The first real network path intentionally separates the legacy application from modern Internet-service requirements:

```text
WebView
  |
  v
WebPlatformHost
  |
  v
RemoteBridgeWebBackend
  |
  v
NetworkRequestExecutor
  |
  v
Win32NetworkRequestExecutor
  |
  v
NetworkTransport
  |
  v
Win32HttpTransport
  |
  v
trusted LAN
  |
  v
modern companion process
  |
  +-- current Browser Probe modern HTTPS fetch
  +-- future service/session adapters
  +-- optional browser/runtime compatibility backend
  `-- other translation/compatibility work
```

`NetworkRequest`, `NetworkResponse`, `NetworkTransport`, and the asynchronous
`NetworkRequestExecutor` contract live in the web/network layer. They do not
expose Winsock handles, Win32 thread handles, or concrete UI types.

`Win32HttpTransport` remains the small blocking Winsock2 implementation.
`Win32NetworkRequestExecutor` is the concrete engine/platform adapter that runs
that blocking transport work away from the UI thread and exposes completion only
through the backend-neutral executor contract.

`RemoteBridgeWebBackend` consumes only `NetworkRequestExecutor`. Its
`navigate()` queues work and returns; its normal `update()` consumes completed
results on the application thread. The worker never mutates views or
`WebSurfaceSnapshot` state directly.

The modern companion is a separate process and may use a modern runtime/toolchain. The
current Browser Probe already delegates modern HTTPS/TLS to that companion while the
legacy machine retains the native application and presentation. Future service/session
or browser-runtime adapters can reuse the same architectural boundary.

This is not a pixel-streaming architecture: the long-term preference is to return
semantic data/events to SalixWeb32 and keep Conversation/Markdown/attachments native
on the P4.

The first bridge protocol is intentionally non-sensitive and must not carry credentials
or session tokens over its current plaintext transport. See `docs/REMOTE_BRIDGE.md`.

## Capability discovery

Backends report supported features instead of pretending all features exist.

The initial capability contract includes:

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

The placeholder backend reports the contract-level features it genuinely supports and reports network/document/script/upload capability as unavailable. Both the Web workspace and Runtime diagnostics surface this information.

The remote bridge backend may report `network` capability because it implements the selected LAN transport path while still reporting HTML, JavaScript, WebSocket, and file upload unavailable until the companion protocol genuinely supplies them.

Longer-term capability reporting can expand, for example:

```text
html_parser        yes
css_grid           no
javascript         yes
websocket          yes
service_worker     no
wasm               no
```

This makes diagnostics on old hardware much more useful and prevents an implementation from silently implying support it does not have.

## Web document abstraction

Long-term goal:

```text
WebDocument
  `-- WebNode
       +-- node type
       +-- attributes
       +-- text
       +-- children
       +-- computed style
       `-- layout metadata
```

This representation should be neutral enough to consume output from native parsers, translation backends, or adapters.

Do not prematurely force it to replicate the complete WHATWG DOM API internally.

## Application framework relationship

SalixTorrent demonstrated a useful pattern where semantic application concepts do not directly depend on the concrete presentation toolkit.

SalixWeb32 preserves that pattern:

```text
Button
TextInput
Container
WebView
        |
        v
ComponentRenderer / framework composition
        |
        v
Win32 implementation
```

This is a conceptual mirror, not a line-for-line port.

Native platform controls can still be used behind framework contracts where they are advantageous. For example, the Win32 backend provides real combo-box and scrollbar peers while the application continues to depend on `ComboBox`, `ScrollBar`, and `NativeControlHost` rather than HWNDs.

The current `WebView` is itself composed from existing framework primitives while consuming only the generic `WebPlatformHost`. A future rendering-heavy WebView may gain additional renderer/surface integration without changing the rule that product code must not depend on a concrete web backend.

## Conversation document relationship

The native messenger shell is also being used to prove document-style presentation concepts before a network/SaaS backend exists.

Canonical message data must remain separate from the component tree used to display it.

```text
MessageDraft / remote message
        |
        v
canonical FormattedText / Markdown
        |
        +-------------------------------+
        | retained for transport/copy   |
        v                               |
MarkdownBlockParser                     |
        |                               |
        +-- text block                  |
        +-- code block                  |
        |                               |
        v                               |
ConversationMessageView                 |
        |                               |
        +-- wrapped Label               |
        +-- CodeBlockView               |
        +-- wrapped Label               |
        |                               |
        v                               |
ConversationView pixel viewport         |
                                        |
canonical source <----------------------+
```

`MarkdownFormatter` remains responsible for inline/text-oriented Markdown semantics. `MarkdownBlockParser` is responsible for block segmentation where separate components are required. Platform renderers draw semantic components; they do not become Markdown parsers.

This distinction matters for future SaaS integration because a provider response may contain prose, lists, inline code, fenced code, links, attachments, and other blocks in one message. The application should be able to retain the provider's canonical payload while choosing an efficient legacy-friendly presentation tree.

### Code blocks

`CodeBlockView` is a composite application/framework presentation object built from ordinary framework components:

```text
CodeBlockView
    +-- header Panel
    |    +-- language Label
    |    `-- Copy Button
    +-- body Panel
    |    `-- selectable code Label
    `-- horizontal ScrollBar
```

The code body uses semantic `TextFormat::code_block` rather than teaching the generic conversation surface to special-case source code. The Win32 text backend maps code semantics to Courier New and suppresses emoticon substitution.

Code blocks keep long logical source lines intact. Horizontal overflow belongs to the code block; vertical conversation scrolling belongs to `ConversationView`.

The current per-code-block horizontal scrollbar deliberately uses the framework fallback inside the clipped conversation viewport. A future nested-native-peer policy must define how child HWNDs are parented/clipped before native scrollbars are used inside vertically scrolled content.

## Python relationship

Python is optional to SalixWeb32 itself.

```text
Runtime
  |
  +-- native services
  |
  `-- optional future PythonHost
          |
          `-- plugins / scripts
```

If Python is absent, the native product still starts and operates.

The Python bridge companion is not an embedded `PythonHost`; it runs on the modern
companion machine and is one implementation of the remote bridge protocol. Any future
effort to port a newer Python runtime to NT5 should be able to live as a separate
project and be consumed through a narrow Salix boundary rather than becoming a build
prerequisite for SalixWeb32.

## Security relationship

Security is not a late plugin.

At minimum the architecture must reserve clear ownership for:

```text
origin identity
certificate validation
cookie policy
CORS
CSP
permissions
secure contexts
sandboxing/isolation
network endpoint auditing
```

A backend that lacks a security capability must report that honestly.

The current LAN bridge is intentionally plaintext and therefore restricted to trusted local-network transport testing. It must not be exposed to the Internet or used for credentials in this state. Later companion-side modern TLS/service work must keep that boundary explicit.

## Presentation-to-interaction invariant

SalixWeb32 may deliberately present a reduced representation when full presentation
would be wasteful or disruptive on constrained hardware. That reduction must remain a
presentation decision rather than a data-loss decision.

The architectural rule is:

> Any reduced presentation must provide a clear path to the complete experience or the
> complete underlying data.

Examples include:

```text
large text
    bounded preview -> Expand / Copy full / Export full

image
    thumbnail -> Preview / Open original

code
    bounded viewport -> scroll / Copy complete source

diagnostics
    concise status -> complete exported report
```

Full-data actions should operate from canonical/cached data rather than reconstructing
content from the visible preview or forcing the complete payload through the renderer.

This allows SalixWeb32 to optimize aggressively for the Pentium 4 without creating a
second-class product experience. See `docs/PRESENTATION_INTERACTION_POLICY.md`.

## Performance assumptions

The baseline target is a Pentium 4 with 2 GB RAM.

Therefore:

- avoid unnecessary multiprocess overhead in the earliest implementation,
- do not assume large memory budgets,
- prefer bounded caches,
- make optional subsystems actually optional,
- instrument memory use,
- avoid background tasks with no user value,
- benchmark on target hardware rather than modern workstations only.

Security requirements can override performance preferences.


## UI/background-work discipline

SalixWeb32 follows the same separation proven in SalixTorrent: background work may
produce data, but presentation state is owned and applied on the application's UI
thread. Network workers do not call framework widgets or mutate view state.

The Browser Probe therefore uses an explicit handoff:

```text
UI thread: Go
    |
    v
RemoteBridgeWebBackend::navigate() queues request
    |
    v
Win32NetworkRequestExecutor worker
    |
    v
blocking NetworkTransport::send()
    |
    v
completion record
    |
    v
UI thread: WebPlatformHost::update()
    |
    v
RemoteBridgeWebBackend publishes new WebSurfaceSnapshot revision
    |
    v
BrowserProbeView observes revision and refreshes once
```

The surface revision is intentionally a polling/version contract rather than an
implicit observer graph. Large Browser Probe payloads are copied into the view
only when the backend publishes a new revision; changing Summary/Headers/Raw/
Extracted operates on the view's cached snapshot and does not call the backend.

### Large-text rendering rule

Network decoupling does not make presentation work free. Browser Probe proved that
large minified HTML can still overwhelm the legacy formatted-text renderer even after
the network request itself is off-thread.

Raw therefore keeps the complete captured body for diagnostics/copy while presenting
only a small hard-wrapped preview. This follows the presentation-to-interaction
invariant above: the preview is bounded, but Copy and diagnostic export still expose the
complete cached data.

The generic Win32 text path has also been improved with per-pass formatted-font reuse
and run-based measurement/drawing. Real Pentium 4 testing reported substantially better
responsiveness; further optimization should remain target-driven rather than expanding
the visible Raw payload merely because rendering became cheaper.
