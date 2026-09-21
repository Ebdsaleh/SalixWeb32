# SalixWeb32

**SalixWeb32** is an experimental modular modern-service/application runtime for legacy and constrained Win32 systems, beginning with Windows Server 2003 SP2 x86 on Pentium 4 hardware.

The immediate proof is a native messenger-style client that can communicate with modern services without requiring the legacy machine to host a contemporary Chromium-class browser runtime. SalixWeb32 keeps application ownership, native presentation, Markdown/code rendering, attachments, diagnostics, and interaction on the legacy machine while allowing expensive compatibility work to live behind interchangeable backends.

The project is deliberately **not** architected around one website, browser engine, or service.

SalixWeb32 also follows a presentation-to-interaction rule: a bounded preview, summary,
thumbnail, or collapsed representation must provide a clear path to the complete
experience or complete underlying data. Performance optimization may reduce what is
rendered initially, but it must not silently remove user capability.

## Development license notice

SalixWeb32 is currently **source-available for inspection and development review, not open source**.

The current development license permits viewing and studying the source, but use, execution, deployment, modification, redistribution, incorporation into another project, derivative works, or other reuse requires the project owner's **prior express written permission**, except for rights necessarily provided through the repository hosting service or applicable law.

The restrictive development license is intended to cover releases through and including **v1.2.0**. A later release is planned to move to a more permissive license, but that future license has not yet been selected and no future rights are granted in advance.

Code contributions are not being accepted during the development-license period unless they are covered by a separate written contribution/relicensing agreement approved by the project owner. Bug reports, testing results, design discussion, and suggestions remain welcome.

See `LICENSE` and `docs/LICENSE_POLICY.md` for the complete terms and project policy.

## Core idea

SalixWeb32 separates:

- the native application shell,
- reusable application/runtime infrastructure,
- backend-neutral framework components,
- Win32 presentation and platform services,
- web/service compatibility,
- and transport/execution backends.

```text
Application
    |
    +-- app/          native product views and message models
    +-- conversation/ semantic service requests/events/backends
    +-- runtime/      lifecycle, services, diagnostics
    +-- framework/    semantic UI/components
    +-- engine/       Win32 host, rendering, platform adapters
    |
    +-- web/
         +-- platform/    backend contracts
         +-- network/     request/response/transport/executor contracts
         +-- security/
         +-- document/
         +-- scripting/
         +-- style/
         +-- layout/
         +-- graphics/
         +-- storage/
         +-- backends/
```

The application does not depend directly on Chromium, Gecko, Python, or a specific SaaS implementation.

The current working path is companion-assisted **for development and proof**:

```text
SalixWeb32 / Pentium 4
        |
        +-- Browser Probe --------------------+
        |                                     |
        `-- ConversationRequest               |
                |                              |
                v                              v
          Win32 HTTP transport ------> salix_bridge.py
                                           |
                                           | localhost
                                           v
                                  salix_chat_session.py
                                           ^
                                           | localhost
                                           |
                                  LibreWolf relay extension
                                           |
                                           v
                                   normal LibreWolf
                                           |
                                           v
                                      chatgpt.com
```

The legacy machine remains the native/semantic application client. The modern companion
is temporary scaffolding used to prove current behavior quickly; it is not the
authoritative product build environment. Where practical, the capabilities proven on the
modern side are intended to be replaced by NT5-native analogues without changing
application-facing contracts.

## Initial target

- CPU: Pentium 4 class, SSE2-era hardware
- RAM: 2 GB
- OS: Windows Server 2003 SP2 x86 / NT 5.2
- IDE/toolchain: Visual Studio .NET 2003 / Visual C++ 7.1 (MSVC 7.1)
- Runtime goal: native Win32 first
- Deferred compatibility target: MiniXP (not an active acceptance gate)
- Python: optional future scripting/plugin capability, never a mandatory SalixWeb32 runtime dependency

## Current state

The native application/framework foundation is operational on the real Pentium 4 target. The project currently includes:

- native `Conversation`, `Browser`, and `Runtime` workspaces,
- multiline message composition and list-aware input behavior,
- append-only native conversation history,
- Markdown-oriented message presentation,
- code-block presentation and copy support,
- attachments, image preview, and file actions,
- native tabs, scrollbars, menus, diagnostics, screenshots, and clipboard integration,
- a UTF-8 framework / UTF-16 Win32 text boundary with Unicode GDI and clipboard support
  currently staged for real-target validation,
- an evolving `Options -> Debug` submenu for one-click target diagnostics/capture/export workflows,
- explicit persistent file-location ownership with configurable diagnostics and remembered attachment-picker history,
- the `WebPlatformBackend` / `WebPlatformHost` abstraction,
- the `ConversationServiceBackend` / `ConversationServiceHost` semantic chat abstraction,
- local and remote conversation backends that emit request/message/text-delta/completion events,
- placeholder and remote bridge web backends,
- backend-neutral network request/response contracts,
- a background Win32 request executor so blocking network transport does not run on the UI thread,
- persistent machine-local bridge configuration,
- a modern-side Python bridge plus a separate localhost-only chat-session broker and
  normal-LibreWolf relay WebExtension,
- and Browser Probe diagnostics for inspecting real modern HTTPS responses.

Browser Probe has been exercised against `https://www.chatgpt.com/` from the real
Server 2003/Pentium 4 target through the companion. The current path retrieves and
reports the real HTTP response, including redirects/headers/document signals, and can
copy the complete captured raw response while keeping the on-screen Raw preview
deliberately bounded for the legacy renderer. Target status is external-service
dependent; a later validation run correctly captured a Cloudflare challenge response
as HTTP 403 rather than treating it as a transport failure.

The current bridge is **unauthenticated** and the P4-to-companion hop is plain HTTP on a trusted, narrowly firewalled LAN. Credentials, cookies, session tokens, private conversations, and uploads must not be carried through this Browser Probe transport yet.

The Win32 formatted-text path has now received two target-driven optimizations: formatted
font reuse and run-based measurement/drawing. Real Pentium 4 testing reported a
substantial responsiveness improvement. Browser Probe still keeps its Raw presentation
bounded because rendering hundreds of kilobytes of minified HTML provides little UX
value; complete Raw remains available through Copy and diagnostic export.

The Conversation workspace now has a provider-neutral semantic service contract. Its
local placeholder path has been exercised on the real Pentium 4, and remote bridge mode
selects a separate `RemoteConversationBackend` proof.

The remote semantic Conversation probe and its explicit probe-only security policy have
completed positive real-target passes. That content-free probe remains available as a
diagnostic path.

The current validated text baseline goes one step further: `salix_bridge.py` can relay
text-only Conversation requests to a localhost-only
`tools/salix_chat_session.py` broker. A small development WebExtension running inside
the user's **normal LibreWolf process** talks to that broker; no Selenium/Marionette
browser process is used. The user authenticates normally in LibreWolf, while Salix does
not receive ChatGPT credentials, cookies, or browser session storage. Only typed message
text and rendered assistant response text cross the trusted development LAN.

That text-only relay is now validated end-to-end on the real Pentium 4 / Windows Server
2003 target. A native SalixWeb32 message reached the authenticated ChatGPT thread through
the companion/WebExtension path, and the real assistant response returned through
`SALIX-CONVERSATION/1` semantic events into the native Conversation view. The target
VC7.1 build remained clean at zero errors and zero warnings.

The bounded file-attachment baseline is now validated in both directions on the real
Pentium 4 / Windows Server 2003 target. Salix can package text/image/generic files behind the
`ConversationServiceBackend` boundary, while assistant-returned files come back as
semantic attachment events and are stored under the active Salix data root. The first
target-validation bounds are 8 files, 2 MB per file, and 4 MB total; credentials and
browser session state remain disabled. LibreWolf extension `0.2.9` is the validated companion baseline for bounded
text/generic/image file relay. Image MIME types use the longer stable settle window before
automatic Send; returned images preserve filename/MIME, suppress duplicate browser
candidates, store one file under `Received`, render inline, and support native Preview
plus external Open on the real Server 2003 target.

The next native candidate moves the existing attachment bounds into a shared
`ConversationAttachmentPolicy` used by both composer and transport. The composer now
shows `Files: N / 8`, disables its attachment button at capacity, and preflights the
8-file / 2 MB-per-file / 4 MB-total limits before Send. Target validation is pending.

The relay still waits for the rendered assistant response to stabilize before the broker
returns the completed response to Salix, so browser-side generation/stabilization remains
a latency target. Native post-response drip-feeding is no longer part of that delay:
Salix drains all semantic events already available from a completed response and
coalesces them into one Conversation presentation update.

End-to-end relay timing is now instrumented and validated on the real P4. The first
captured target baseline measured about 19.4 seconds on the modern relay versus
57.5 seconds to native `message_completed`, exposing roughly 38.1 seconds of artificial
native-side post-response pacing. The corrected batching path is now target-validated:
a completed response carrying 27 semantic deltas drained into one native presentation
update, measured presentation was 0 ms, and P4 completion followed bridge completion by
only about 184 ms. The corresponding VC7.1 build completed with zero errors and zero
warnings.

The text path now keeps framework/wire content as UTF-8, makes
wrapping/navigation code-point aware, and converts to UTF-16 only at the native Win32
presentation/clipboard boundary. The real P4 validation round-tripped
`I’m “testing” — café € → ↓` through Conversation rendering, Unicode clipboard,
composer paste, and the real browser relay without mojibake. Salix also performs
glyph-aware Win32 font fallback when the preferred Tahoma/Courier New face cannot draw
a span; the final Server 2003 retest rendered the arrow glyphs correctly. The matching
VC7.1 build completed with zero errors and zero warnings.

The modern companion is development scaffolding and a behavioral reference, not the
authoritative SalixWeb32 build environment. The product executable and required native
runtime remain targeted at the Pentium 4 / Windows Server 2003 / Visual C++ 7.1
environment. Companion-assisted capabilities are expected to be replaced by NT5-native
analogues where practical as those implementations mature.

File storage now follows the same explicit-ownership philosophy. File dialogs no longer
own process-wide path state: the attachment picker uses `OFN_NOCHANGEDIR`, diagnostics
receive an explicit configured directory, and `Options -> Settings...` exposes only
actual storage policy. Attachment browsing keeps its most recent directory automatically
in `settings.ini`; first use starts at `%USERPROFILE%`. Standard launches keep writable
application state under `%APPDATA%\SalixWeb32`; launching with `--portable` deliberately
moves that state beside `SalixWeb32.exe`.

## Documentation

Start with:

- `ROADMAP.md`
- `ARCHITECTURE.md`
- `docs/BROWSER_PROBE.md`
- `docs/CONVERSATION_SERVICE_CONTRACT.md`
- `docs/REMOTE_BRIDGE.md`
- `docs/CHAT_SESSION_RELAY.md`
- `docs/WEB_BACKEND_CONTRACT.md`
- `docs/BUILD_ENVIRONMENT.md`
- `docs/DEPENDENCY_STRATEGY.md`
- `docs/PRESENTATION_INTERACTION_POLICY.md`
- `docs/FILE_LOCATIONS.md`
- `docs/VALIDATION.md`
- `docs/CODING_STYLE.md`
- `docs/LICENSE_POLICY.md`

Feature-specific documents under `docs/` record the native conversation, composer, attachment, Markdown, selection, scrolling, syntax-highlighting, and menu behavior.

## License

SalixWeb32 is currently distributed under the **Salix Development Source License 1.0** in `LICENSE`.

It is intentionally restrictive during active development and is **not an OSI-approved open-source license**. The project owner intends to adopt a more permissive license for a release after v1.2.0; the final post-development license has not yet been selected.
