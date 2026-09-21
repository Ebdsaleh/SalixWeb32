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

The next Conversation-liveness tranche is planned to add semantic request/provider status
without exposing provider DOM objects. It separates:

```text
request transport:
    transmitting / verified / provider_accepted / failed

provider generation:
    idle / generating / stabilizing / completed /
    cancelled / provider_failed / connection_lost

provider composer:
    unavailable / ready / submitting / followup_ready
```

This distinction matters because a provider can still be generating while its composer is
already available for another user message. A follow-up is therefore another immutable
`ConversationRequest` with its own request ID and receipt verification, not a mutation
of the original request.

The service backend returns semantic content rather than remote UI state. Provider DOM
objects, browser automation objects, Python implementation objects, and provider-specific
response types do not cross this boundary.

The first `PlaceholderConversationBackend` performs no network access and has validated
the request/event lifecycle and native streaming presentation on the P4.

The application preserves semantic event order without requiring one presentation rebuild
per event. When multiple `text_delta` events are already available in the same backend
update, `StatusView` drains that available batch, accumulates the canonical text, and
coalesces native Conversation presentation. The completed-response browser relay has
validated this on the real P4 with 27 deltas producing one presentation update. Future
true streaming remains incremental because only newly arrived events can be drained.

`RemoteConversationBackend` reuses the existing backend-neutral network contracts but
owns a **separate Win32 request executor/HTTP transport instance** from Browser Probe.
This avoids Browser and Conversation single-flight/lifecycle contention while allowing
both to reach the same companion host/port. The original content-free probe remains a
diagnostic endpoint. Bounded text/generic/image attachment framing is now validated
end-to-end behind `RemoteConversationBackend`, which emits semantic `attachment` events
without exposing browser upload mechanics to the application. The active native candidate
adds composer-side preflight UX while keeping the same relay bounds.

Before accepting a real Conversation request, `RemoteConversationBackend` asynchronously
checks `GET /v1/health`. The companion must advertise the exact
`SALIX-CONVERSATION/1` browser-relay policy and report the localhost LibreWolf session
ready. Backend readiness is therefore negotiated rather than inferred from generic TCP
reachability or Browser Probe success.

The planned liveness extension takes this one step further. Readiness and an active
request's health will be reported independently: bridge reachability, companion outbound
connectivity, WebExtension heartbeat, provider generation state, and provider composer
state are distinct observations. Provider-specific Send/Stop/Follow-up/error UI signals
remain inside the browser adapter and are translated into provider-neutral Conversation
status.

Long-running generation will not be failed merely because a short wall-clock timer
expired while these liveness signals remain healthy.

`ApplicationRuntime` explicitly initializes, updates, and shuts down the optional
`ConversationServiceHost` alongside the optional `WebPlatformHost`. Event consumption
and `ConversationView` mutation remain on the application thread.

See `docs/CONVERSATION_SERVICE_CONTRACT.md`.

## Text encoding boundary

Application, conversation, Markdown, and clipboard MIME text is canonically stored as
UTF-8 bytes.

The framework keeps its existing byte-indexed `std::string` representation, but
wrapping/navigation/hit-testing treat valid UTF-8 sequences as atomic code points.
The Win32 presentation backend converts UTF-8 spans to UTF-16 and uses Unicode GDI APIs
for measurement/drawing. The Win32 clipboard similarly maps framework UTF-8 to
`CF_UNICODETEXT`.

```text
framework/service UTF-8
        |
        v
Win32Utf8Text
        |
        v
UTF-16
        |
        +-- Unicode GDI
        `-- CF_UNICODETEXT
```

Legacy ACP strings that still enter from older Win32 `...A` platform APIs are accepted
as a compatibility fallback only when the source bytes are not valid UTF-8. New
application/service text should remain UTF-8.

Font glyph availability is separate from encoding correctness. See
`docs/TEXT_ENCODING.md`.

## Persistent file-location boundary

SalixWeb32 does not treat the process current working directory as durable application
state. Startup resolves immutable application roots before any file dialog can run.

```text
                    STANDARD                         PORTABLE (--portable)

executable_root     executable directory             executable directory

user_data_root      %APPDATA%\SalixWeb32            executable directory

settings.ini        user_data_root\settings.ini      user_data_root\settings.ini

Diagnostics         user_data_root\Diagnostics       user_data_root\Diagnostics

Attachment picker   recent navigation history          recent navigation history
```

The launch directory is still captured once for development/local-config discovery. It
is not a persistent application storage root.

Attachment navigation is deliberately not a user-facing storage preference. The picker
remembers its last successful directory in `settings.ini`; when no history exists, its
first-use directory is `%USERPROFILE%`. If that environment value is unavailable, the
captured launch directory and then the executable directory are fallback locations.

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
  +-- Browser Probe modern HTTPS fetch
  +-- salix_bridge.py
  |      |
  |      `-- localhost salix_chat_session.py broker
  |              ^
  |              |
  |       LibreWolf relay WebExtension
  |              |
  |              `-- normal LibreWolf / current ChatGPT session
  `-- other temporary reference/compatibility work
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

The modern companion is a separate development/reference process and may use a modern
runtime/toolchain. It is scaffolding used to prove current service behavior while the
legacy machine retains the native application and presentation. It is not the
authoritative SalixWeb32 product build environment.

The current ChatGPT baseline returns semantic text/events to SalixWeb32 and keeps
Conversation/Markdown presentation native on the P4. Pixel transport is not forbidden,
but Salix is not being turned into a general remote-desktop client.

The bridge must not carry ChatGPT credentials, cookies, or browser session tokens. The
validated baseline permits message text; the current dev candidate additionally permits
explicitly bounded attachment file bytes while leaving credentials/session material
disabled. See
`docs/REMOTE_BRIDGE.md` and `docs/CHAT_SESSION_RELAY.md`.

Conversation dispatch has an additional backend-neutral security gate:

```text
ConversationRequest
        |
        v
ConversationServiceHost
        |
        v
ConversationSecurityProfile
        |
        +-- probe-only
        |      `-> backend receives request ID only
        |
        `-- content
               |
               +-- local-process                 -> eligible
               +-- trusted-lan                   -> explicit development use only
               +-- authenticated-encrypted      -> eligible
               `-- plaintext                    -> rejected
```

The profile also declares whether text, attachments, credentials, and session state are
allowed. The current `RemoteConversationBackend` is content/trusted-lan with text
enabled and attachments/credentials/session disabled. The local placeholder is
content/local-process. The original probe-only/plaintext request remains available as a
content-free regression path.

The `trusted-lan` category is deliberately explicit: it records that the current
browser-relay proof sends message text across the user's tightly scoped development LAN
without pretending that transport is cryptographically protected.

The authoritative native product remains built with VC7.1 on the P4/NT5 target.
Modern-machine Python/browser tooling is reference scaffolding, not a source of required
modern-compiler DLLs for the SalixWeb32 executable.

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

The current LAN bridge is intentionally restricted to trusted local-network testing. It
must not be exposed to the Internet or used for credentials. The validated browser-relay
baseline permits message text; the active dev candidate also permits bounded attachment
file bytes under explicit capability negotiation. Authentication/session material
remains inside LibreWolf. Future native security work must keep that boundary explicit.

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
