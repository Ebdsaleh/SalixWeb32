# Remote Bridge Transport

This document records the first real transport path between SalixWeb32 on the legacy machine and a modern companion process.

## Architectural role

The bridge is a **development/reference backend**, not a replacement for SalixWeb32's long-term native communications path.

The primary product goal remains:

```text
Pentium 4 / Windows Server 2003
        |
        v
SalixWeb32 native networking / TLS / HTTP / service adapters
        |
        v
modern Internet services
```

The bridge remains useful as a test oracle, compatibility backend, diagnostics path, and a way to inspect modern HTTPS behavior while native pieces are still being built.

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
NetworkTransport
        |
        v
Win32HttpTransport (Winsock2, plain HTTP on trusted LAN)
        |
        v
modern companion: tools/salix_bridge.py
```

The application still knows only `WebPlatformBackend`. Winsock details remain in the Win32 transport and companion protocol details remain in the remote backend.

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

The companion does not execute target JavaScript and does not claim to provide a browser DOM.

See `docs/BROWSER_PROBE.md` for the Browser Probe workflow and limits.

## Backend selection

The placeholder backend remains the default.

To opt into the remote backend:

```text
SALIX_WEB_BACKEND=remote
SALIX_BRIDGE_HOST=<modern-machine-ip-or-hostname>
SALIX_BRIDGE_PORT=8765
```

`SALIX_BRIDGE_PORT` defaults to `8765`.

For the P4-to-modern-machine test, set `SALIX_BRIDGE_HOST` to the modern machine's LAN address.

## Starting the companion

On the modern machine:

```text
python tools/salix_bridge.py --host 0.0.0.0 --port 8765
```

Binding to `0.0.0.0` permits LAN access. Keep the host firewall rule narrowly scoped to the Pentium 4 source address.

The current startup banner reports both bridge and Browser Probe protocol versions.

## Starting SalixWeb32

On the Pentium 4:

```text
set SALIX_WEB_BACKEND=remote
set SALIX_BRIDGE_HOST=<modern-machine-LAN-IP>
set SALIX_BRIDGE_PORT=8765
bin\Debug\SalixWeb32.exe
```

The Browser Probe no longer performs a real Internet fetch during SalixWeb32 startup. The user explicitly presses `Go` in the Browser workspace.

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

That validation applies to the transport foundation. Browser Probe v1's new `/v1/fetch` behavior still requires its own real P4 validation.

## Current limits

The bridge is intentionally simple:

- plain HTTP on the private LAN,
- IPv4 Winsock2 client transport,
- bounded response size,
- explicit requests only,
- no background polling,
- no authenticated service data.

These limits keep the bridge understandable while SalixWeb32 learns which modern web/service capabilities are actually required.
