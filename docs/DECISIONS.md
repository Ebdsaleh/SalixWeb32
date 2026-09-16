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
