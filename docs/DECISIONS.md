# Architecture Decision Log

## ADR-001 — Use `SalixWeb32` as working name

**Status:** Accepted

Reason: broader and more accurate than a chat-specific name; communicates Salix family + Win32 focus.

---

## ADR-002 — Native-first runtime

**Status:** Accepted

The core application/runtime must run without Python or a modern browser runtime.

---

## ADR-003 — Browser-engine independence

**Status:** Accepted

LibreWolf/Firefox/Gecko are reference/possible provider technologies, not the identity of the application.

---

## ADR-004 — Web platform is modular

**Status:** Accepted

Networking, security, document, scripting, style, layout, graphics, and storage are explicit capability areas.

---

## ADR-005 — Support multiple web backend families

**Status:** Accepted

Initial architectural families:

- native
- Gecko-derived
- translator
- remote

---

## ADR-006 — Python is optional

**Status:** Accepted

A successful CPython port may become a scripting/plugin host and a bridge to existing Salix Python work, but SalixWeb32 must not require it to start.

---

## ADR-007 — Do not self-implement high-risk primitives by default

**Status:** Accepted

Cryptography/TLS and JavaScript execution should preferentially use mature, auditable implementations where feasible.

---

## ADR-008 — Build for the real target

**Status:** Accepted

The Pentium 4 / 2 GB / Server 2003 x86 machine is the baseline reality, not an afterthought.

---

## ADR-009 — Restrictive source-available development license

**Status:** Accepted

SalixWeb32 uses the `Salix Development Source License 1.0` during active development.

The policy is intentionally source-available rather than open source: public source may be inspected and studied, but execution, deployment, modification, redistribution, incorporation into other projects, derivative works, and other reuse require prior express written permission except where repository-hosting terms or applicable law necessarily provide narrower rights.

The restrictive development-license period is intended to cover releases through and including `v1.2.0`. A later release is intended to move to a more permissive license, but the future license is not selected in advance and no future rights are granted automatically.

Outside copyrightable contributions are not accepted by default during this period unless separately approved under written contribution/relicensing terms. This preserves the project's ability to change licensing cleanly later.

See `LICENSE` and `docs/LICENSE_POLICY.md`.


---

## ADR-010 — Keep the legacy machine as the semantic application client

**Status:** Accepted

SalixWeb32 should keep application state, native conversation presentation, Markdown,
attachments, diagnostics, and ordinary interaction on the legacy machine.

A modern companion may temporarily provide modern browser/service capabilities as
development scaffolding and a behavioral reference. The preferred application-facing
shape is semantic data/events because that preserves a useful native Win32 application,
but pixels are not forbidden when they are genuinely the simplest compatibility proof.

The companion is not the authoritative product build environment. Where practical,
capabilities proven there should later be replaced by NT5-native analogues.

---

## ADR-011 — Background workers do not mutate UI state

**Status:** Accepted

Blocking network work may execute on a platform worker, but worker threads do not call
framework widgets or mutate application presentation state.

The current web path uses:

```text
RemoteBridgeWebBackend::navigate()
    -> NetworkRequestExecutor::submit()
    -> Win32 background worker
    -> blocking NetworkTransport
    -> completion record
    -> WebPlatformHost::update() on application thread
    -> backend publishes new surface revision
    -> BrowserProbeView refreshes on application thread
```

This mirrors the broader Salix separation between background work and presentation
ownership and avoids implicit callback graphs crossing thread boundaries.

---

## ADR-012 — Large diagnostic payloads use bounded presentation

**Status:** Accepted

The complete canonical payload may be retained for diagnostics/copy while the visible
legacy UI presents a smaller bounded representation.

Browser Probe Raw currently keeps the complete captured response but displays only a
small hard-wrapped preview. This is a deliberate protection for constrained hardware,
not permission to ignore renderer performance. The generic Win32 text path remains a
profiling/optimization target.

---

## ADR-013 — Machine-local bridge configuration stays out of source control

**Status:** Accepted

Bridge host/port/backend selection can be stored in `salixweb32.local.ini` for
repeatable target launches. That file is ignored by Git. The repository contains only
`salixweb32.local.ini.example`.

Environment variables remain supported as overrides, and no machine-specific address,
credential, token, or private session material belongs in the repository.


---

## ADR-014 — Reduced presentation must preserve a path to full fidelity

**Status:** Accepted

SalixWeb32 may use snippets, summaries, thumbnails, bounded previews, collapsed sections,
lazy presentation, or other reduced representations to protect responsiveness on
constrained hardware.

Such a representation must provide a clear and content-appropriate path to the complete
experience or complete underlying data whenever that full data exists.

Examples:

- Browser Probe Raw: bounded on-screen preview, complete Copy and Export.
- image attachments: thumbnail, then Preview/Open original.
- code: bounded/scrollable presentation, complete source retained for Copy.
- diagnostics: concise status, complete report export.

Truncation and transformation are presentation policies, not permission to discard
canonical data. Full-data actions should use canonical/cached state directly and should
not require rendering the full payload first.

See `docs/PRESENTATION_INTERACTION_POLICY.md`.


---

## ADR-015 — Conversation services emit semantic events

**Status:** Accepted

Conversation providers are isolated behind `ConversationServiceHost` and
`ConversationServiceBackend`.

Application-facing communication uses provider-neutral requests and semantic events such
as `request_started`, `message_started`, `text_delta`, `message_completed`, and
`request_failed`.

The native Conversation UI must not depend on provider DOM objects, browser automation
objects, Python implementation objects, or provider-specific response structures.

Backends may internally use an API, a browser/session runtime, a translator, a native
network stack, or another mechanism, but those details stop at the backend boundary.
Presentation updates are consumed on the application thread.

See `docs/CONVERSATION_SERVICE_CONTRACT.md`.


---

## ADR-016 — Plaintext Conversation bridge begins with a content-free probe

**Status:** Accepted

The first `RemoteConversationBackend` proof must not forward typed draft text,
attachment paths, credentials, cookies, tokens, or session state over the current
plaintext trusted-LAN bridge.

It sends only a Salix request ID and fixed zero-forwarding flags to
`/v1/conversation/probe`. The companion returns framed semantic events using
`SALIX-CONVERSATION/1`.

This validates the P4 -> companion -> semantic event -> native Conversation pipeline
without quietly weakening the security boundary. Real conversation content requires a
separately designed secure credential/session transport.


---

## ADR-017 — Remote Conversation readiness is capability-negotiated

**Status:** Accepted

A reachable bridge host and a working Browser Probe do not imply that the running
companion supports the current Conversation protocol.

`RemoteConversationBackend` therefore performs an asynchronous `GET /v1/health`
handshake and requires:

```text
conversation_probe=enabled
conversation_protocol=SALIX-CONVERSATION/1
```

before accepting a Conversation probe.

This decision follows the first real remote Conversation target pass, where Browser Probe
remained healthy but `POST /v1/conversation/probe` returned HTTP 404. Capability
negotiation turns that deployment/version mismatch into explicit application state rather
than presenting the backend as ready and failing only after Send.


---

## ADR-018 — Persistent file categories own explicit directories

**Status:** Accepted

The process current working directory is not a persistent-storage contract.

A Win32 common file dialog may change that directory as a side effect, so diagnostics,
exports, attachments, caches, and future file categories must not communicate implicitly
through it.

SalixWeb32 therefore:

- captures launch and executable directories once at startup,
- decides Standard versus Portable mode once from the process command line,
- uses `%APPDATA%\SalixWeb32` as `user_data_root` in Standard mode,
- uses the executable directory as `user_data_root` only for `--portable`,
- stores `settings.ini` and the default `Diagnostics` directory beneath that root,
- keeps user-facing preferences separate from development bridge configuration,
- treats Diagnostics as a user-configurable location,
- treats the attachment directory as automatic recent-navigation state rather than a
  user-facing preference,
- uses `%USERPROFILE%` as the first-use attachment location when no history exists,
- uses `OFN_NOCHANGEDIR` for the attachment picker,
- passes the diagnostics destination explicitly to diagnostic exporters,
- exposes the first path controls through `Options -> Settings...`.

Standard mode does not silently fall back to launch/executable storage if APPDATA cannot
be resolved. Portable storage is an explicit launch-time opt-in.

The attachment picker is intentionally frictionless: its most recent successful
directory is persisted automatically in `attachment_directory`, but that history is not
shown as an editable Settings field. A fixed "always open here" override is deferred
unless a concrete workflow demonstrates the need for one.

See `docs/FILE_LOCATIONS.md`.

---

## ADR-019 — Probe dispatch and content dispatch are separate security paths

**Status:** Accepted

`ConversationServiceHost` must not hand a `ConversationRequest` to a backend merely
because that backend can participate in the semantic Conversation contract.

Every backend exposes a `ConversationSecurityProfile` containing:

- dispatch mode: blocked, probe-only, or content,
- transport security: none, local-process, plaintext, trusted-LAN, or authenticated-encrypted,
- explicit permission flags for text, attachments, credentials, and session state.

Probe-only backends are invoked through `submit_probe(request_id)`. The request object
is not passed to them at all.

Content backends are invoked through `submit_request(request, request_id)` only when
the profile allows the requested data classes. Local-process and
authenticated-encrypted transports are normal content paths. An explicitly labelled
`trusted_lan` transport may carry narrowly scoped development content when the user has
chosen that test topology; ordinary `plaintext` remains ineligible.

The current browser-relay baseline uses `trusted_lan` for message text only.
Attachments, credentials, cookies, and browser session state remain disallowed.

This creates a fail-closed boundary for future service adapters: adding a new backend
does not automatically authorize private conversation data. A network backend must
explicitly satisfy the secure profile before the host will pass content to it.

Cryptography/TLS remains subject to ADR-007: SalixWeb32 should use a mature, auditable
implementation rather than inventing cryptographic primitives in project code.

See `docs/CONVERSATION_SERVICE_CONTRACT.md`.

---

## ADR-020 — Modern TLS behind a newer-toolchain provider DLL

**Status:** Superseded

An earlier experiment proposed a `SalixSecureTransport.dll` built with a newer
NT5-capable Microsoft toolset. That direction was rejected because it would make a
modern compiler part of the required SalixWeb32 runtime-production path and did not match
the project's authoritative build goal.

The associated provider/test-DLL implementation was removed.

---

## ADR-021 — Modern companion is temporary reference/scaffolding

**Status:** Accepted

The authoritative SalixWeb32 native target is the Pentium 4 / Windows Server 2003 /
Visual C++ 7.1 environment.

The modern machine may run Python, a current browser, or other tooling to prove current
service behavior quickly. Those tools are reference/scaffolding infrastructure rather
than the intended permanent Salix runtime.

For the first useful ChatGPT baseline:

```text
SalixWeb32 / P4
    -> SALIX-CONVERSATION/1 text request
    -> salix_bridge.py
    -> localhost salix_chat_session.py
    -> visible user-authenticated LibreWolf / chatgpt.com
    -> rendered assistant text
    -> semantic Conversation events
    -> native Salix ConversationView
```

Authentication remains inside the visible browser. The Salix protocol does not transport
ChatGPT credentials, cookies, or browser session storage.

The first pass uses whichever ChatGPT thread is already open in the worker-owned browser.
Conversation selection, true byte-streaming response transport, and attachments are
separate later tranches.

As NT5-native equivalents become practical, companion responsibilities should migrate
back toward the legacy platform without changing the application-facing conversation
contract.

