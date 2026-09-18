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

A modern companion may provide expensive compatibility capabilities such as modern TLS,
service-specific protocols, or a browser/runtime adapter, but the preferred architecture
is to return semantic data/events rather than remote-rendered pixels.

This preserves a useful native Win32 application instead of turning SalixWeb32 into a
thin remote-desktop/browser viewport.

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
