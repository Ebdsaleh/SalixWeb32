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
  `-- RemoteBackend
```

`WebPlatformHost` is the backend-selection and lifecycle boundary. It can be given one concrete backend by the composition root before runtime initialization. Consumers see only generic navigation, surface, input, capability, and identity operations.

The application must not branch on Gecko-specific types, backend-specific DOM objects, remote-bridge packet structures, or any other concrete provider detail.

The first Phase 3 implementation uses `PlaceholderWebBackend`. It intentionally performs no network activity; its purpose is to prove the entire dependency direction on VC7.1 and the real target before a real web engine is selected.

See `docs/WEB_BACKEND_CONTRACT.md` for the concrete Phase 3 contract and validation checklist.

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

Python is optional:

```text
Runtime
  |
  +-- native services
  |
  `-- optional PythonHost
          |
          `-- plugins / scripts / migrated Salix logic
```

If Python is absent, the native product still starts and operates.

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
