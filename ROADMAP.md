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
- [x] Create initial Git repository
- [x] Select development license (`Salix Development Source License 1.0`)
- [x] Establish coding conventions
- [x] Add first tagged baseline once skeleton builds (`v0.0.1`)

The current license is deliberately source-available and restrictive during active development. It is intended to cover releases through and including `v1.2.0`; a later release is planned to adopt a different and more permissive license. See `LICENSE` and `docs/LICENSE_POLICY.md`.

**Exit criterion:** repository exists with documentation and agreed source layout.

## Phase 1 — Native Win32 application skeleton

Goal: prove the Salix architecture without any browser engine.

- [x] Create VS.NET 2003-compatible solution/project
- [x] Native `WinMain`
- [x] `ApplicationRuntime`
- [x] service lifecycle (`start/update/stop`)
- [x] `ServiceRegistry`
- [x] diagnostics/logging foundation
- [x] `Win32ApplicationHost`
- [x] main window creation
- [x] resize handling
- [x] clean shutdown
- [x] basic status reporting

The Phase 1 native skeleton and hardening tranche have been validated on the target Pentium 4 under Windows Server 2003 SP2 x86. The executable builds with Visual C++ 7.1, launches from both Visual Studio and Command Prompt, exercises the runtime service lifecycle, updates live status without repaint ghosting, tracks client-area resizing correctly, and exits cleanly. The baseline executable also passed a MiniXP smoke test on the same hardware.

See `docs/VALIDATION.md` for target-hardware results.

**Exit criterion:** achieved. A native x86 executable runs correctly on Server 2003 SP2 and cleanly opens/closes a window.

## Phase 2 — Framework / presentation separation

Goal: reproduce the useful architectural separation demonstrated by SalixTorrent.

- [x] semantic `Component` base
- [x] container component
- [x] label
- [x] button
- [x] text input
- [x] basic row/column layout concepts
- [x] event dispatch
- [x] styling primitives
- [x] `ComponentRenderer` contract
- [x] Win32 component renderer
- [x] application view independent of Win32 calls
- [x] styled `Panel` composition
- [x] reusable `MessageInputStrip`
- [x] messenger-style shell layout
- [x] double-buffered Win32 presentation
- [x] text caret navigation/editing
- [x] mouse/keyboard text selection
- [x] copy/cut/paste
- [x] backend-neutral clipboard contract
- [~] MIME-aware composer payloads
- [x] precise text hit-testing
- [x] Ctrl+Arrow word-boundary navigation
- [x] selectable read-only labels
- [x] double-click word / triple-click line selection
- [x] discontinuous/additive text selection
- [x] subtractive selection gestures
- [x] compact vs preserved-spacing clipboard payloads
- [x] paste modes (`Ctrl+V` compact / `Ctrl+Shift+V` keep formatting)
- [x] compact vs keep-formatting cut modes
- [x] bounded text undo/redo
- [x] append-only conversation model
- [x] scrollable conversation view
- [ ] framework dirty-region invalidation

The advanced Phase 2 text/editing baseline was validated on the Pentium 4 under both Windows Server 2003 SP2 and MiniXP and tagged as:

```text
v0.0.2
```

The validated baseline includes discontinuous selection, additive/subtractive mouse workflows, dual clipboard representations, compact/keep-formatting cut/paste modes, and bounded Undo/Redo state restoration.

### Post-v0.0.2 composer and conversation presentation

The messenger shell now uses a dedicated `MessageComposer`, richer conversation document presentation, and increasingly reusable framework/platform capabilities rather than growing one monolithic text control.

Implemented or active post-baseline work:

- [~] `MessageComposer` composition root
- [~] `MessageToolbar`
- [~] toggle-button primitive
- [~] native-backed font-size combo-box primitive
- [~] backend-neutral multi-file dialog contract
- [~] Win32 `GetOpenFileNameA` Explorer-style multi-file provider
- [~] native Ctrl+Click / Shift+Click multi-file attachment selection
- [~] semantic attachment model and draft plumbing
- [~] removable composer attachment chips/tray
- [~] inline image attachment thumbnails with aspect-ratio preservation
- [~] attachment Preview/Open/context actions
- [~] image Preview mouse-wheel zoom
- [~] attachment-aware plain conversation selection/copy surrogate
- [~] native application menu bar (`File / Edit / Options / Help`)
- [~] evolving `Options -> Debug` diagnostics submenu for target-hardware feedback
- [x] reusable framework `TabView` with native Win32 tab peer
- [x] Conversation/Runtime tab lifecycle on Server 2003 SP2 and MiniXP
- [~] Bold/Italic/Underline composer state (`Ctrl+B`, `Ctrl+I`, `Ctrl+U`)
- [~] formatted text model and formatted sent-message preservation
- [~] multiline composer with explicit line-break rendering/hit-testing
- [~] native composer horizontal/vertical viewport scrollbars
- [~] delta-synchronized native scrollbar peers
- [~] composer mouse-wheel scrolling (`Wheel` vertical / `Shift+Wheel` horizontal)
- [~] persistent visual-column Up/Down caret navigation
- [~] backend-neutral context-menu model with native Win32 popup presentation
- [~] composer right-click Copy/Cut/Paste + keep-formatting variants
- [~] cross-block read-only selection within one conversation message presentation
- [~] document-wide read-only conversation selection and Copy/Select All
- [~] bulleted/numbered list editing and dedicated `ListPanel`
- [~] list continuation with Enter and `Ctrl+Enter` forced send
- [~] classic text-emoticon registry
- [~] `EmojiPanel` composite popup
- [~] graphical classic-messenger-inspired emoticon rendering provider
- [~] code composer mode (`Ctrl+;`) with 2/4/6/8-space indentation
- [~] shared code-language registry and native composer language selector
- [~] Markdown conversation presentation
- [~] inline/fenced code semantics with literal emoticons in code
- [~] native conversation scrollbar and mouse-wheel input
- [~] soft-wrapped, pixel-scrolled conversation viewport
- [~] block-level Markdown segmentation for mixed messages
- [~] dedicated `CodeBlockView` with language header and Copy button
- [~] non-wrapped code with independent horizontal overflow scrolling
- [~] lightweight language-aware code syntax tokenization/highlighting foundation
- [ ] advanced syntax grammar / richer syntax palette
- [ ] structured clipboard representation for inline attachment objects
- [ ] clickable Markdown links
- [ ] richer Markdown block widgets (quotes/tables/task lists/images)

The classic emoticon registry stores text aliases such as `:)`, `:D`, `;)`, `:P`, `:'(`, and `<3` as canonical message content while the Win32 presentation layer can draw original classic-messenger-inspired graphical faces. Code semantics suppress that substitution so the same aliases remain literal source text inside inline or fenced code.

The native tabbed workspace baseline was historically exercised on the real Pentium 4
under both Windows Server 2003 SP2 and MiniXP. Active acceptance is now Server 2003 R2
only; MiniXP compatibility work is deferred until the Server 2003 feature set is complete
and the MiniXP environment is usable again. The later native-menu/attachment work has
rebuilt and run green on Server 2003, including multi-file selection, removable
attachment chips, inline image presentation, internal Preview, default-application Open,
Preview mouse-wheel zoom, and native P4 outgoing file relay. The current `<` / `>`
overflow controls are functionally accepted for now but their visual interaction is
explicitly deferred to a later UI-polish pass.

Known VC7.1-era SDK gotchas are recorded in `docs/BUILD_ENVIRONMENT.md`, including the recurring local `WM_MOUSEWHEEL` compatibility definition, common-controls include-order requirements, and Winsock2-before-`windows.h` rule.

See `docs/COMPOSER_FORMATTING.md`, `docs/EMOTICON_RENDERING.md`, `docs/MULTILINE_COMPOSER.md`, `docs/CODE_COMPOSER.md`, `docs/MARKDOWN_RENDERING.md`, `docs/RICH_CONVERSATION_VIEWPORT.md`, `docs/CONVERSATION_SCROLLING.md`, `docs/CODE_BLOCKS.md`, `docs/SYNTAX_HIGHLIGHTING.md`, `docs/CONTEXT_MENUS.md`, `docs/TABBED_VIEWS.md`, `docs/NATIVE_MENU_BAR.md`, and `docs/ATTACHMENTS.md` for the current contracts and validation checklists.

**Exit criterion:** core Phase 2 criterion achieved at `v0.0.2`. Post-baseline composer/conversation work continues without changing the validated baseline tag.

## Phase 3 — WebView and backend contract

Goal: web support has a stable slot before any engine is chosen.

- [x] `WebView` semantic component
- [x] `WebPlatformBackend` contract
- [x] backend lifecycle through optional `WebPlatformHost`
- [x] navigation request model
- [x] first document/render surface snapshot contract
- [x] input/event bridge
- [x] placeholder backend
- [x] diagnostics capability reporting
- [x] backend selection mechanism

Initial backend families:

```text
placeholder   development/validation only
native
gecko
translator
remote
```

The first Phase 3 source tranche introduced the third web workspace through
`PlaceholderWebBackend`. That workspace has since evolved into the `Browser`
workspace and can also be supplied by `RemoteBridgeWebBackend`. The placeholder
remains useful as the no-network fallback while the remote backend exercises the same
generic contract with a real transport.

The September 16, 2026 Pentium 4 / Windows Server 2003 SP2 target pass rebuilt this tranche with Visual C++ 7.1 with **no build errors or warnings**. The running Web workspace correctly reported the placeholder identity/family/lifecycle, explicit capabilities, requested ChatGPT address, and changing forwarded-input count.

See `docs/WEB_BACKEND_CONTRACT.md`.

**Exit criterion:** achieved on the primary Server 2003 target. The shell displays a `WebView` supplied by a dummy backend without product code knowing the concrete backend type. MiniXP remains a separate smoke target.

## Phase 4 — Web foundation primitives

Goal: implement or port reusable pieces that every web backend needs.

- [ ] URL parsing / canonicalization
- [~] MIME handling foundation (`MimeData` introduced by the UI/clipboard layer)
- [ ] Unicode/text encoding abstraction
- [ ] stream/buffer abstraction
- [x] network request/response model
- [x] backend-neutral `NetworkTransport` contract
- [x] backend-neutral asynchronous `NetworkRequestExecutor` contract
- [x] Win32 background request executor for blocking transports
- [x] Win32 Winsock2 plain-HTTP transport for trusted-LAN bridge validation
- [x] remote companion protocol and modern-side development server
- [x] persistent machine-local bridge configuration
- [ ] cookie model
- [ ] cache abstraction
- [ ] certificate/trust abstraction
- [ ] security origin model
- [ ] content decoding/compression investigation

The transport foundation is now validated on the real P4. Blocking Winsock work is
owned by a Win32 worker behind `NetworkRequestExecutor`; completion is consumed on
the normal application update thread. The current P4-to-companion link deliberately
remains plain HTTP on a trusted LAN and must not carry credentials.

See `docs/REMOTE_BRIDGE.md` and `docs/BROWSER_PROBE.md`.

**Exit criterion:** substantially achieved for the transport/request foundation.
Security/document primitives remain active work rather than prerequisites for using the
remote compatibility backend.

## Phase 5 — Network + HTTPS viability

Goal: retrieve modern HTTPS content through an explicitly selected secure/compatibility
path while preserving a future native option.

### Companion-assisted path

- [x] trusted-LAN P4 -> companion transport
- [x] companion-side modern HTTPS GET
- [x] redirects and response headers
- [x] IPv4-only companion fetch mode for the validated host
- [x] bounded response capture
- [x] sensitive response-header redaction
- [x] real `https://www.chatgpt.com/` HTTP 200 proof from the P4
- [x] Browser Probe summary/headers/raw/extracted diagnostics
- [x] off-UI-thread legacy transport execution
- [ ] authenticated service/session transport
- [ ] secure credentials boundary
- [ ] streaming semantic service events

### Native/direct path

This remains a legitimate future backend rather than the current blocking prerequisite.

- [ ] qualify portable TLS candidates
- [ ] verify NT 5.2/x86 runtime compatibility
- [ ] direct modern HTTPS from the P4
- [ ] certificate validation/trust policy
- [ ] proxy support as required
- [ ] investigate HTTP/2 only if a target service requires it
- [ ] defer HTTP/3 unless a concrete target requires it

**Exit criterion:** achieved for unauthenticated modern HTTPS inspection through the
remote compatibility path. Authenticated service communication remains pending.

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

Possible approaches may coexist behind stable Salix contracts:

```text
service/API backend
web-session compatibility backend
native/direct backend
translation backend
remote companion backend
```

The P4 remains the native application client. The modern companion is temporary
development scaffolding and a behavioral reference: it proves modern service/browser
behavior now so Salix can progressively replace those capabilities with NT5-native
analogues. The current ChatGPT baseline relays semantic text through a visible browser
session rather than turning SalixWeb32 into a general remote-desktop client.

- [x] conversation display with semantic local/remote message updates
- [x] text input -> provider-neutral `ConversationRequest` handoff
- [~] attachments (semantic local model, image presentation, and native P4 outgoing
  transfer validated; returned-file capture candidate active)
- [~] bounded file transfer through Conversation backend (outgoing green; reverse
  returned-file byte transport green; 0.2.7 semantic filename/deduplication retest pending)
- [x] copy/paste large diagnostic text through the native clipboard path
- [~] authenticated web-session relay through visible LibreWolf on the companion
- [~] semantic response streaming into `ConversationView` (local + remote probe proof;
  browser relay currently returns a completed response then releases semantic deltas)
- [ ] conversation-thread selection from Salix
- [ ] session persistence
- [x] diagnostics panel and native screenshot/report capture
- [~] privacy/network endpoint visibility foundation
- [x] remote bridge backend/companion transport foundation
- [x] Browser Probe modern HTTPS inspection
- [~] legacy large-text rendering/performance hardening

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

Python remains optional to SalixWeb32 and is not a prerequisite for the native product.

A future modern-Python-on-NT5 effort is expected to be developed as a separate project
rather than turning SalixWeb32 into the Python port itself. SalixWeb32 may later consume
such a runtime through a narrow scripting/plugin boundary.

- [ ] define optional scripting host contract if/when required
- [ ] integrate a separately validated Python runtime provider
- [ ] expose selected Salix services safely to scripts
- [ ] keep SalixWeb32 fully operational when the provider is absent

Historical experiment: the CPython 3.13.3 source archive was unpacked on Server 2003
and the official modern x86 installer was rejected by that OS as not being a valid
Win32 application. Further porting work belongs outside the SalixWeb32 critical path.

**Exit criterion:** optional scripts/plugins can run through a provider boundary;
SalixWeb32 still works when the provider is absent.

## Phase 13 — Portability / productization

- [ ] Windows XP x86 testing
- [~] Windows Server 2003 variants
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

## Semantic conversation-service foundation

The first provider-neutral conversation-service contract is now implemented:

- [x] `ConversationRequest` for text and attachment references,
- [x] generated Salix request IDs,
- [x] `ConversationEvent` semantic stream vocabulary,
- [x] `ConversationServiceBackend` provider contract,
- [x] `ConversationServiceHost` lifecycle/selection boundary,
- [x] local placeholder backend with no network access,
- [x] in-place native Remote-message updates from `text_delta` events,
- [x] real P4 runtime validation of the placeholder semantic stream,
- [x] remote conversation backend + `SALIX-CONVERSATION/1` probe protocol implemented,
- [x] real P4 <-> companion validation of the remote semantic probe,
- [x] companion Conversation capability/version handshake,
- [x] successful end-to-end `SALIX-CONVERSATION/1` target pass,
- [x] explicit probe-only/plaintext security-policy negotiation and sensitive-data
  forwarding assertions implemented,
- [x] validate the probe-only security-policy gate on the real P4,
- [x] machine-readable `ConversationSecurityProfile` and separate probe/content dispatch
  boundary implemented,
- [x] validate the host content-dispatch boundary on the real P4,
- [x] preserve the content-free Conversation probe as a regression/diagnostic path,
- [x] evolving native Debug diagnostics submenu implemented and target-validated,
- [x] add a localhost-only chat-session broker on the modern companion,
- [x] add a normal-LibreWolf development WebExtension so ChatGPT relay does not require
  Selenium/GeckoDriver/Marionette browser automation,
- [x] add a trusted-LAN text-only browser-relay path without forwarding credentials,
  cookies, browser session state, or attachments,
- [x] validate the complete modern-side LibreWolf relay round-trip with a real ChatGPT
  message and returned semantic events,
- [x] validate the LibreWolf browser relay end-to-end on the real P4,
- [ ] add conversation-thread selection/new-thread control,
- [x] instrument end-to-end browser-relay latency and validate it on the modern
  companion plus the real P4 with a clean VC7.1 build,
- [x] establish and real-P4 validate the UTF-8 framework / UTF-16 Win32 presentation
  boundary, Unicode clipboard round-trip, and glyph-aware Win32 font fallback,
- [x] batch native Conversation delta presentation; corrected real-P4 retest drained
  27 semantic deltas into 1 native presentation update with 0 ms measured presentation,
- [ ] optimize browser-relay latency after architecture/version freeze,
- [ ] convert the relay from completed-response framing to true incremental transport,
- [~] validate bounded bidirectional Conversation file relay (8 files, 2 MB each,
  4 MB total) with semantic attachment events and application-owned Received storage,
- [ ] progressively replace companion capabilities with NT5-native equivalents where
  practical.

See `docs/CONVERSATION_SERVICE_CONTRACT.md`.

**September 19, 2026 milestone:** the complete text-only path is target-green on the real
Pentium 4 / Server 2003 machine: native SalixWeb32 request -> trusted-LAN bridge ->
localhost broker -> normal-LibreWolf WebExtension -> authenticated ChatGPT thread ->
assistant response -> semantic events -> native Salix Conversation view.

The current release candidate is **v0.0.4**, following the existing v0.0.3 native
conversation/rich-composer milestone. v0.0.4 freezes the first-contact architecture.
Post-baseline relay timing, UTF-8/UTF-16 text handling, glyph fallback, and native
completed-response batching have now also been validated on the real P4. A bounded
bidirectional file-relay candidate is active on `dev`; true streaming, thread selection,
and further latency work remain follow-up tranches.

The first real timing pass on `dev/test` measured about 19.4 s for the modern relay but
57.5 s to native `message_completed` on the P4, leaving roughly 38.1 s outside the
modern relay window. The corrected batching tranche is now real-P4 validated: the
completed response produced 27 semantic deltas but only 1 native presentation update,
with 0 ms measured presentation and only about 184 ms between bridge completion and
native completion. The artificial post-response drip-feed penalty is therefore removed.

## Persistent file-location foundation

The first explicit file-location policy is now implemented:

- [x] capture launch and executable directories once at process startup,
- [x] resolve an immutable Standard/Portable application mode,
- [x] store Standard writable state under `%APPDATA%\SalixWeb32`,
- [x] store Portable writable state beside the executable when launched with `--portable`,
- [x] use `settings.ini` and `Diagnostics\` beneath the selected data root,
- [x] keep development bridge configuration separate from user-facing preferences,
- [x] add `Options -> Settings...` for the user-configurable Diagnostics folder,
- [x] make the attachment picker use `OFN_NOCHANGEDIR`,
- [x] keep attachment navigation history out of Settings,
- [x] use `%USERPROFILE%` as the first-use attachment picker location,
- [x] persist the last successful attachment directory automatically,
- [x] pass the diagnostics directory explicitly to screenshot/report exporters,
- [~] validate Standard and Portable storage behavior on Server 2003 R2 / Pentium 4,
- [ ] defer MiniXP compatibility validation until the Server 2003 feature set is complete
  and the MiniXP environment is repaired/stabilized.

Future Downloads, Cache, Session Export, and Conversation Export categories should extend
the same explicit `user_data_root` model rather than reusing the process current
directory.

See `docs/FILE_LOCATIONS.md`.

## Current near-term priority

1. keep the real P4 / Server 2003 R2 build clean under VC7.1,
2. complete the active Server 2003 R2 validation of Standard and Portable persistent
   file locations,
3. harden response extraction and failure diagnostics against ordinary page changes,
4. optimize relay latency without weakening the current boundaries,
5. add explicit conversation-thread selection after one-current-thread relay is green,
6. validate the bounded bidirectional file-relay candidate on Server 2003 R2,
7. move from completed-response framing to true incremental response transport,
8. continue using the companion as a reference/scaffold while replacing its capabilities
   with NT5-native implementations where practical,
9. keep Browser Probe and the content-free Conversation probe as regression tools,
10. defer MiniXP compatibility work until the Server 2003 feature set is complete and
    the MiniXP environment is usable again.

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
