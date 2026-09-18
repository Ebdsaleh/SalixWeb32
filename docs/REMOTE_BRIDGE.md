# Remote Bridge Transport

This document records the first real transport path between SalixWeb32 on the legacy machine and a modern companion process.

## Architectural role

The bridge is currently the project's **primary compatibility path** for capabilities
that are unreasonable to force into the Pentium 4 process, while remaining one backend
behind Salix-owned contracts.

The architectural goal is not remote pixels. SalixWeb32 keeps the native application,
conversation/document presentation, attachments, clipboard behavior, and diagnostics on
the legacy machine. The companion can supply modern TLS, service/session behavior, or a
future browser/runtime adapter and return semantic data/events.

A native/direct modern-network backend remains a valid future option, but it is no
longer a prerequisite for progressing the application.

## Transport architecture

```text
SalixWeb32 application / Browser Probe
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
Win32NetworkRequestExecutor (background worker)
        |
        v
NetworkTransport
        |
        v
Win32HttpTransport (Winsock2, plain HTTP on trusted LAN)
        |
        v
modern companion: tools/salix_bridge.py
```

The application still knows only `WebPlatformBackend`. The remote backend sees
only the backend-neutral request-executor contract; Win32 thread ownership stays
in the engine/platform adapter and Winsock details remain in the Win32 transport.
Companion protocol details remain in the remote backend.

## Current companion endpoints

The companion now supports:

```text
GET  /v1/health
POST /v1/navigate
POST /v1/fetch
```

`/v1/navigate` remains the original transport-proof acknowledgement endpoint.

`/v1/fetch` is used by Browser Probe v1. It receives an absolute `http://` or `https://` URL as `text/plain`, performs an unauthenticated modern HTTPS GET on the companion machine, and returns a framed `SALIX-PROBE/1` diagnostic payload containing:

- requested URL,
- final URL after redirects,
- upstream HTTP status,
- MIME type,
- captured response size,
- redirect count,
- lightweight HTML signal counts,
- response headers,
- raw textual response content,
- lightweight extracted text/title.

The current companion does not execute target JavaScript and does not claim to provide
a browser DOM. A future browser/runtime compatibility adapter would be a separate
capability behind the same architectural boundary rather than a change to the native
Salix UI model.

See `docs/BROWSER_PROBE.md` for the Browser Probe workflow and limits.

## Backend selection

The placeholder backend remains the fallback when no bridge configuration is present.

For day-to-day development, copy `salixweb32.local.ini.example` to
`salixweb32.local.ini` in the repository root and set the companion address:

```text
web_backend=remote
bridge_host=<modern-machine-ip-or-hostname>
bridge_port=8765
```

The local file is ignored by Git. SalixWeb32 checks the current working directory
first and then `..\\..\\salixweb32.local.ini`, which allows a Visual Studio .NET
2003 launch from `build\\vs2003` to reuse the repository-root file.

Environment variables remain supported and override file values:

```text
SALIX_WEB_BACKEND=remote
SALIX_BRIDGE_HOST=<modern-machine-ip-or-hostname>
SALIX_BRIDGE_PORT=8765
```

`SALIX_CONFIG` can point to a different local settings file. If no explicit
backend is configured, supplying a bridge host automatically selects the remote
backend. `SALIX_BRIDGE_PORT` defaults to `8765`.

## Starting the companion

On the modern machine:

```text
python tools/salix_bridge.py --host 0.0.0.0 --port 8765
```

Binding to `0.0.0.0` permits LAN access. Keep the host firewall rule narrowly scoped to the Pentium 4 source address.

The current startup banner reports both bridge and Browser Probe protocol versions.

## Starting SalixWeb32

The normal development path uses the ignored repository-root
`salixweb32.local.ini` created from `salixweb32.local.ini.example`. This lets
Visual Studio .NET 2003 launch the correct remote backend without re-entering
environment variables for each shell.

Environment variables remain supported as explicit overrides.

The Browser Probe does not perform a real Internet fetch during SalixWeb32 startup.
The user explicitly presses `Go` in the Browser workspace.

Remote mode uses a longer bounded bridge receive timeout so the companion has enough time to complete a modern HTTPS request while still failing cleanly if the companion or target becomes unresponsive.

## Security boundary

The P4-to-companion transport is still intentionally **plain HTTP**.

Therefore Browser Probe v1 is deliberately unauthenticated:

- no ChatGPT credentials,
- no authorization headers,
- no browser cookies,
- no session tokens,
- no private conversation payloads,
- no file uploads.

The companion also redacts sensitive response headers such as `Set-Cookie` before returning probe headers over the plaintext LAN link.

Only use this path on the existing trusted, tightly firewalled development LAN.

## Validated transport result

The September 16, 2026 target pass validated the original bridge transport between the real Pentium 4 / Windows Server 2003 SP2 client and the Windows Server 2022 companion machine.

Observed results included:

- companion listening on `0.0.0.0:8765`,
- local and P4 `/v1/health` success,
- 0% packet loss between the P4 and companion host during the test,
- bounded failure while TCP 8765 was blocked,
- successful connection after an inbound firewall rule was restricted to the P4 source address,
- successful `POST /v1/navigate`,
- HTTP 200 returned to SalixWeb32,
- preserved `https://www.chatgpt.com/` navigation target,
- clean round-trip through `WebView -> WebPlatformHost -> RemoteBridgeWebBackend -> NetworkTransport -> Win32HttpTransport -> companion` and back.

That transport validation has since been extended by real Browser Probe
`/v1/fetch` validation. On September 18, 2026, the P4 successfully requested
`https://www.chatgpt.com/` through the companion and received a real upstream HTTP
200 HTML result. The validated probe reported a final `https://chatgpt.com/` URL,
one redirect, 497574 captured bytes, 16 script signals, one form, and 11 links.

Raw export correctness was also verified: the copied Raw section contained the actual
HTML document rather than the response-header block.

## Current limits

The bridge is intentionally simple:

- plain HTTP on the private LAN,
- IPv4 Winsock2 client transport,
- one bounded background request at a time,
- explicit user-triggered requests only,
- completion consumed from the normal application update thread,
- no background polling of the companion,
- no authenticated service data.

These limits keep the bridge understandable while SalixWeb32 learns which modern
web/service capabilities are actually required.

The next bridge-level expansion should be semantic service/session work, not an
unbounded increase in Browser Probe payload size. Any credential-bearing path must first
define a secure boundary; the current plaintext LAN protocol is deliberately excluded
from that role.
