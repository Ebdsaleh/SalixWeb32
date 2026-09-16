# Remote Bridge Transport

This document records the first real transport path between SalixWeb32 on the legacy machine and a modern companion process.

## Goal

The immediate goal is deliberately smaller than "load the modern web":

> prove that the Pentium 4 can send a backend-neutral request over the LAN to a modern process and receive a response without exposing modern service/TLS details to the SalixWeb32 application shell.

The architecture is:

```text
SalixWeb32 application / WebView
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
        |
        v
future modern TLS / service / translation logic
```

The application still knows only `WebPlatformBackend`. Winsock details stay in the Win32 transport and the remote bridge protocol stays in the remote backend/companion boundary.

## What this tranche does

The legacy client gains:

- `NetworkRequest`
- `NetworkResponse`
- `NetworkTransport`
- `Win32HttpTransport`
- `RemoteBridgeWebBackend`
- explicit runtime backend selection through environment variables

The modern side gains the dependency-free Python companion:

```text
tools/salix_bridge.py
```

The first protocol supports:

```text
GET  /v1/health
POST /v1/navigate
```

`POST /v1/navigate` receives the requested URL as `text/plain` and returns a small `SALIX-BRIDGE/1` acknowledgement.

This is a **real TCP/HTTP transport test**, but the companion intentionally does not fetch the URL yet.

## What this tranche does not do

It does not yet:

- authenticate to ChatGPT or another SaaS provider,
- send user chat messages,
- upload files,
- download remote assets,
- proxy HTML,
- execute JavaScript,
- provide WebSocket/SSE streaming,
- provide TLS on the legacy machine.

The point is to validate the transport seam before placing modern service behavior on top of it.

## Backend selection

The existing placeholder remains the default.

With no environment variables set:

```text
SalixWeb32 -> PlaceholderWebBackend
```

To opt into the bridge backend:

```text
SALIX_WEB_BACKEND=remote
SALIX_BRIDGE_HOST=<modern-machine-ip-or-hostname>
SALIX_BRIDGE_PORT=8765
```

`SALIX_BRIDGE_PORT` is optional and defaults to `8765`.

`SALIX_BRIDGE_HOST` defaults to `127.0.0.1`, but that is only useful when the companion is running on the same Windows machine. For the intended Pentium-4-to-modern-PC test, set it to the modern machine's LAN address or resolvable hostname.

The remote backend is opt-in so a missing companion cannot accidentally make the normal placeholder development path depend on network availability.

## Starting the modern companion

On the modern machine, from the repository root:

```text
python tools/salix_bridge.py --host 0.0.0.0 --port 8765
```

The server prints its protocol and listen address.

Binding to `0.0.0.0` allows another machine on the LAN to connect. The host firewall may require an inbound private-network rule for TCP port `8765`.

For a local-only test, omit `--host` and the server defaults to `127.0.0.1`.

## Starting SalixWeb32 with the bridge backend

For the first target test, the simplest route is a Command Prompt on the Pentium 4:

```text
set SALIX_WEB_BACKEND=remote
set SALIX_BRIDGE_HOST=192.168.x.x
set SALIX_BRIDGE_PORT=8765
bin\Debug\SalixWeb32.exe
```

Replace `192.168.x.x` with the modern companion machine's LAN address.

If Visual Studio is launched before those environment variables are defined, a debug-launched child process may not inherit them. Running the already-built executable from the configured Command Prompt avoids that ambiguity during the first test.

## Expected Web tab when connected

The `Web` workspace should identify:

```text
Backend: Remote Bridge Web Backend
family: remote
network: yes
```

The initial navigation target remains:

```text
https://www.chatgpt.com/
```

The remote backend sends that target to the companion's `/v1/navigate` endpoint. A successful response should show a bridge body similar to:

```text
SALIX-BRIDGE/1
status=accepted
target=https://www.chatgpt.com/
note=transport boundary verified; modern fetch is not enabled yet
```

This confirms that the request crossed from the legacy client into the modern companion and back.

## Failure behavior

If the companion is not running or the address/port is wrong, SalixWeb32 should still launch.

The Web tab should expose a transport failure such as connection refused/timeout rather than hiding the problem or crashing the application.

The first Win32 transport uses a short bounded connect/read/write timeout so an unreachable companion cannot block indefinitely.

## Security boundary

The first bridge transport is intentionally **plain HTTP**.

That is acceptable only because this tranche is a private-LAN architecture/transport proof. It must not be exposed directly to the public Internet and must not carry credentials, session cookies, access tokens, or private service payloads in this state.

Use it only on a trusted local network while validating this tranche.

Modern Internet TLS and provider authentication belong on the modern companion side in later work. The legacy machine should not be forced to impersonate a modern browser/TLS stack merely to prove the bridge architecture.

## Why the companion is separate

This design keeps three concerns independent:

```text
legacy UI / interaction
        !=
bridge transport
        !=
modern service implementation
```

A future ChatGPT service adapter can live behind the modern companion without changing composer, conversation presentation, attachment UI, or the generic `WebPlatformBackend` seam.

Likewise, a future native TLS backend can coexist with the remote bridge rather than requiring an architectural rewrite.

## Target validation checklist

### Build/lifecycle

1. Close/reopen Visual Studio .NET 2003 because the `.vcproj` gains new translation units and `ws2_32.lib`.
2. Clean and rebuild Debug Win32 with VC7.1.
3. Confirm zero compile/link errors; record warnings separately if any appear.
4. Launch normally with no bridge environment variables and confirm the existing Placeholder backend still behaves exactly as before.

### Offline remote mode

5. Set `SALIX_WEB_BACKEND=remote` with the companion stopped.
6. Launch the executable and confirm SalixWeb32 still reaches the normal shell.
7. Confirm the Web tab identifies the remote backend and reports the bounded connection failure rather than hanging/crashing.
8. Confirm Conversation and Runtime remain usable after the failed request.

### Real LAN round-trip

9. Start `tools/salix_bridge.py` on a modern machine using `--host 0.0.0.0 --port 8765`.
10. Set the Pentium 4 bridge host to that machine's LAN address.
11. Launch SalixWeb32 in remote mode.
12. Confirm the companion console logs a `POST /v1/navigate` request from the Pentium 4.
13. Confirm the Web tab shows HTTP 200 and the `SALIX-BRIDGE/1` response body.
14. Confirm the returned target is `https://www.chatgpt.com/`.
15. Switch Conversation -> Web -> Runtime repeatedly and confirm normal UI/native-control lifecycle remains stable.
16. Close SalixWeb32 and confirm Winsock/backend/runtime shutdown is clean.

Only after the VC7.1 build and real LAN round-trip pass should this transport tranche be marked target-validated.
