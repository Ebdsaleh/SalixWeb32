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

It delegates to a web-platform provider through a backend contract.

```text
WebView
  |
  v
WebPlatformBackend
  |
  +-- NativeBackend
  +-- GeckoBackend
  +-- TranslatorBackend
  `-- RemoteBackend
```

The application must not branch on Gecko-specific types or browser-specific DOM objects.

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

A Gecko backend may internally delegate many of them to Gecko. A native backend may compose Salix implementations. A translator may receive an already transformed document representation.

## Capability discovery

Backends should eventually be able to report supported features rather than pretending all features exist.

Conceptual example:

```text
html_parser        yes
css_grid           no
javascript         yes
websocket          yes
service_worker     no
wasm               no
```

This will make diagnostics on old hardware much more useful.

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

SalixWeb32 should preserve that pattern:

```text
Button
TextInput
Container
WebView
        |
        v
ComponentRenderer
        |
        v
Win32 implementation
```

This is a conceptual mirror, not a line-for-line port.

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
