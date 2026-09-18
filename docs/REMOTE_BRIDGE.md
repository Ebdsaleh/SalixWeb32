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
SalixWeb32
   |
   +-- Browser Probe
   |      |
   |      v
   |   RemoteBridgeWebBackend
   |      |
   |      v
   |   Win32NetworkRequestExecutor
   |      |
   |      v
   |   Win32HttpTransport
   |
   `-- Conversation
          |
          v
      RemoteConversationBackend
          |
          v
      Win32NetworkRequestExecutor
          |
          v
      Win32HttpTransport
          |
          +-------------------+
                              |
                              v
                  trusted-LAN companion
                  tools/salix_bridge.py
```

The Browser and Conversation paths use separate executor/transport instances so their
single-flight workers and lifecycle do not interfere with one another. Both depend on
the same generic `NetworkRequestExecutor` / `NetworkTransport` contracts and may target
the same configured companion host/port.

Win32 thread ownership stays in the engine/platform adapters and Winsock details remain
in `Win32HttpTransport`. Companion protocol details remain inside the selected remote
backends.

## Current companion endpoints

The companion now supports:

```text
GET  /v1/health
POST /v1/navigate
POST /v1/fetch
POST /v1/conversation/probe
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

`/v1/conversation/probe` returns a framed `SALIX-CONVERSATION/1` semantic event
sequence. Its request is intentionally content-free: Salix sends only a generated
request ID plus fixed `text_forwarded=0` and `attachments_forwarded=0` flags. The
companion rejects probe payloads that attempt to enable either field.

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

The current startup banner reports the bridge, Browser Probe, and Conversation protocol versions.

`GET /v1/health` also advertises:

```text
conversation_probe=enabled
conversation_protocol=SALIX-CONVERSATION/1
conversation_mode=probe_only
conversation_text_forwarding=disabled
conversation_attachment_forwarding=disabled
conversation_credential_forwarding=disabled
conversation_session_forwarding=disabled
conversation_transport_security=plaintext
```

The P4 remote Conversation backend checks those values asynchronously during startup.
This prevents an older/stale companion process from being presented as Conversation-ready
merely because the shared host/port is reachable. Readiness also requires the explicit
probe-only security profile; a companion that claims content forwarding or a different
transport-security state is rejected rather than trusted implicitly.

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

Therefore the Browser Probe and remote Conversation **probe** paths are deliberately
non-sensitive:

- no ChatGPT credentials,
- no authorization headers,
- no browser cookies,
- no session tokens,
- no private conversation payloads,
- no draft message text on the Conversation probe,
- no attachment paths/counts on the Conversation probe,
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

An earlier September 19, 2026 Conversation target pass confirmed that Browser Probe and
the remote Conversation backend can be selected at the same time, but a stale companion
returned HTTP 404 for `POST /v1/conversation/probe`. That failure drove the explicit
Conversation capability/version handshake.

A later September 19 target pass with the current restarted companion closed the positive
path:

- `GET /v1/health` returned HTTP 200,
- two `POST /v1/conversation/probe` requests returned HTTP 200,
- the P4 diagnostic reported
  `Remote Conversation Bridge Backend | SALIX-CONVERSATION/1 ready`,
- the same run kept the Remote Bridge Web Backend initialized,
- Browser Probe independently returned a real ChatGPT HTTP 200 response.

The semantic probe is therefore validated end-to-end on the real Server 2003/Pentium 4
target. The next gate is security: real draft text, attachments, credentials, and session
material remain blocked until an approved content-capable transport profile exists.

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

The bridge now has a semantic Conversation probe in addition to Browser Probe. The next
bridge-level expansion should be the secure service/session boundary rather than
forwarding real conversation content over this plaintext protocol. Any credential-bearing path must first
define a secure boundary; the current plaintext LAN protocol is deliberately excluded
from that role.
