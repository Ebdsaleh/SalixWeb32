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

The native tabbed workspace baseline has been exercised successfully on the real Pentium 4 under both Windows Server 2003 SP2 and MiniXP. The later native-menu/attachment work has also rebuilt and run green on the Pentium 4 under Windows Server 2003 SP2, including multi-file selection, removable attachment chips, inline image presentation, internal Preview, default-application Open, and Preview mouse-wheel zoom. The current `<` / `>` overflow controls are functionally accepted for now but their visual interaction is explicitly deferred to a later UI-polish pass. The latest attachment tranche must not be treated as MiniXP-validated until that repeat smoke pass is explicitly performed.

Known VC7.1-era SDK gotchas are recorded in `docs/BUILD_ENVIRONMENT.md`, including the recurring local `WM_MOUSEWHEEL` compatibility definition and common-controls include-order requirements.

See `docs/COMPOSER_FORMATTING.md`, `docs/EMOTICON_RENDERING.md`, `docs/MULTILINE_COMPOSER.md`, `docs/CODE_COMPOSER.md`, `docs/MARKDOWN_RENDERING.md`, `docs/RICH_CONVERSATION_VIEWPORT.md`, `docs/CONVERSATION_SCROLLING.md`, `docs/CODE_BLOCKS.md`, `docs/SYNTAX_HIGHLIGHTING.md`, `docs/CONTEXT_MENUS.md`, `docs/TABBED_VIEWS.md`, `docs/NATIVE_MENU_BAR.md`, and `docs/ATTACHMENTS.md` for the current contracts and validation checklists.

**Exit criterion:** core Phase 2 criterion achieved at `v0.0.2`. Post-baseline composer/conversation work continues without changing the validated baseline tag.

## Phase 3 — WebView and backend contract

Goal: web support has a stable slot before any engine is chosen.

- [~] `WebView` semantic component
- [~] `WebPlatformBackend` contract
- [~] backend lifecycle through optional `WebPlatformHost`
- [~] navigation request model
- [~] first document/render surface snapshot contract
- [~] input/event bridge
- [~] placeholder backend
- [~] diagnostics capability reporting
- [~] backend selection mechanism

Initial backend families:

```text
placeholder   development/validation only
native
gecko
translator
remote
```

The first Phase 3 source tranche now provides a third `Web` workspace tab backed by `PlaceholderWebBackend`. The shell targets `https://www.chatgpt.com/` through the generic navigation contract, but the placeholder deliberately performs **no network request**. Its job is to prove the application -> framework -> web-platform boundary, backend lifecycle, input forwarding, capability discovery, and diagnostics on VC7.1 before any real engine/network provider is selected.

This tranche is **pending real-target rebuild/validation**. See `docs/WEB_BACKEND_CONTRACT.md`.

**Exit criterion:** shell displays a WebView supplied by a dummy backend without product code knowing the concrete backend type. This criterion is implemented in source but is not marked achieved until the VC7.1/Server 2003 target pass succeeds.

## Phase 4 — Web foundation primitives

Goal: implement or port reusable pieces that every web backend needs.

- [ ] URL parsing / canonicalization
- [~] MIME handling foundation (`MimeData` introduced by the UI/clipboard layer)
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

- [~] conversation display (native shell proof advancing before network backend)
- [~] text input (native shell proof advancing before network backend)
- [~] attachments (semantic local model + image presentation active; transfer backend pending)
- [ ] file transfer
- [~] copy/paste build logs (local UI path exists; remote bridge pending)
- [ ] session persistence
- [~] diagnostics panel
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
