# SalixWeb32

**SalixWeb32** is an experimental modular modern-service/application runtime for legacy and constrained Win32 systems, beginning with Windows Server 2003 SP2 x86 on Pentium 4 hardware.

The immediate proof is a native messenger-style client that can communicate with modern services without requiring the legacy machine to host a contemporary Chromium-class browser runtime. SalixWeb32 keeps application ownership, native presentation, Markdown/code rendering, attachments, diagnostics, and interaction on the legacy machine while allowing expensive compatibility work to live behind interchangeable backends.

The project is deliberately **not** architected around one website, browser engine, or service.

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

The current working path is companion-assisted:

```text
SalixWeb32 / Pentium 4
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
Win32HttpTransport
        |
        v
trusted-LAN companion
        |
        v
modern TLS / compatibility execution / service adapter
```

The legacy machine remains the semantic client. The companion supplies capabilities that are unreasonable or unsafe to force into the P4 process. A future native backend may move selected capabilities back onto the legacy machine without changing application-facing contracts.

## Initial target

- CPU: Pentium 4 class, SSE2-era hardware
- RAM: 2 GB
- OS: Windows Server 2003 SP2 x86 / NT 5.2
- IDE/toolchain: Visual Studio .NET 2003 / Visual C++ 7.1 (MSVC 7.1)
- Runtime goal: native Win32 first
- Secondary smoke target: MiniXP
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
- the `WebPlatformBackend` / `WebPlatformHost` abstraction,
- placeholder and remote bridge backends,
- backend-neutral network request/response contracts,
- a background Win32 request executor so blocking network transport does not run on the UI thread,
- persistent machine-local bridge configuration,
- a modern-side Python companion,
- and Browser Probe diagnostics for inspecting real modern HTTPS responses.

Browser Probe has been exercised against `https://www.chatgpt.com/` from the real Server 2003/Pentium 4 target through the companion. The current path can retrieve a real HTTP 200 HTML response, report redirects/headers/document signals, expose lightweight extracted text, and copy the complete captured raw response while keeping the on-screen Raw preview deliberately bounded for the legacy renderer.

The current bridge is **unauthenticated** and the P4-to-companion hop is plain HTTP on a trusted, narrowly firewalled LAN. Credentials, cookies, session tokens, private conversations, and uploads must not be carried through this Browser Probe transport yet.

The next performance concern is the Win32 formatted-text/rendering path: large/minified text remains much more expensive than ordinary application content on the Pentium 4. Browser Probe now protects the UI with a small Raw preview, but the generic renderer still needs profiling/optimization before very large text should feel completely natural.

## Documentation

Start with:

- `ROADMAP.md`
- `ARCHITECTURE.md`
- `docs/BROWSER_PROBE.md`
- `docs/REMOTE_BRIDGE.md`
- `docs/WEB_BACKEND_CONTRACT.md`
- `docs/BUILD_ENVIRONMENT.md`
- `docs/DEPENDENCY_STRATEGY.md`
- `docs/VALIDATION.md`
- `docs/CODING_STYLE.md`
- `docs/LICENSE_POLICY.md`

Feature-specific documents under `docs/` record the native conversation, composer, attachment, Markdown, selection, scrolling, syntax-highlighting, and menu behavior.

## License

SalixWeb32 is currently distributed under the **Salix Development Source License 1.0** in `LICENSE`.

It is intentionally restrictive during active development and is **not an OSI-approved open-source license**. The project owner intends to adopt a more permissive license for a release after v1.2.0; the final post-development license has not yet been selected.
