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
