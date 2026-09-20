# Remote Bridge Transport

This document records the first real transport path between SalixWeb32 on the legacy machine and a modern companion process.

## Architectural role

The bridge is **development scaffolding and a behavioral reference**, not the intended
final build/runtime dependency for SalixWeb32.

SalixWeb32 keeps the native application, conversation/document presentation, attachments,
clipboard behavior, and diagnostics on the legacy machine. The modern companion exists
so current browser/service behavior can be proven quickly, measured, and then replaced by
NT5-native analogues where practical.

The current ChatGPT baseline uses semantic text relay rather than a general remote
desktop. A localhost-only broker communicates with a small WebExtension running inside
the user's normal LibreWolf process; the bridge brokers only message text and rendered
assistant response text to/from the P4.

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
POST /v1/conversation/message
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

The content-free probe remains a regression/diagnostic path.

`/v1/conversation/message` is the browser-relay content path. The validated baseline
carries text only; the current dev candidate also carries bounded attachment file bytes.
The bridge forwards the framed Salix request to `salix_chat_session.py` over localhost.
That broker queues the request for the LibreWolf relay WebExtension, which uses the
ChatGPT conversation currently open in the user's normal browser process. Returned
assistant text becomes `text_delta` events and returned files become semantic
`attachment` events for the native P4 client.

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

Run LibreWolf normally with the profile already used for ChatGPT.

Load the development relay extension from:

```text
about:debugging#/runtime/this-firefox
```

Choose **Load Temporary Add-on...** and select:

```text
tools\librewolf_chat_relay_extension\manifest.json
```

Reload/open the desired ChatGPT thread after loading the extension so its content script
is active.

Start the localhost broker:

```text
python tools/salix_chat_session.py
```

It binds only to `127.0.0.1:8766` and does not launch or remotely control LibreWolf.

In a second terminal start the P4-facing listener:

```text
python tools/salix_bridge.py --host 0.0.0.0 --port 8765
```

Binding the bridge to `0.0.0.0` permits LAN access. Keep the host firewall rule narrowly
scoped to the Pentium 4 source address. Port 8766 remains localhost-only.

`GET /v1/health` advertises the current relay state:

```text
conversation_probe=enabled
conversation_relay=enabled
conversation_protocol=SALIX-CONVERSATION/1
conversation_mode=browser_relay
conversation_text_forwarding=enabled
conversation_attachment_forwarding=enabled
conversation_credential_forwarding=disabled
conversation_session_forwarding=disabled
conversation_transport_security=trusted_lan
conversation_browser_session=ready
```

The final field becomes `ready` only when the localhost broker has a recent heartbeat
from the WebExtension and the extension can see the ChatGPT composer in an open tab.


## Security boundary

The P4-to-companion transport is still ordinary HTTP on the explicitly trusted
development LAN.

Browser Probe remains non-authenticated and continues to redact sensitive response
headers.

The validated baseline permitted message text only. The current dev candidate permits
message text plus explicitly bounded attachment file bytes (8 files, 2 MB each, 4 MB
total) after capability negotiation. It still does not forward:

- ChatGPT credentials or MFA material,
- authorization headers,
- browser cookies,
- browser/session storage,
- service tokens.

Authentication and session ownership remain entirely inside normal LibreWolf on the
modern machine. The health contract labels this transport `trusted_lan` rather
than pretending it is authenticated/encrypted. This is a development policy, not a claim
that plain HTTP is cryptographically secure.

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
target.

The text-only LibreWolf browser relay is validated end-to-end on the real
Server 2003/Pentium 4 target. The active dev candidate extends that path with bounded
file attachments in both directions; that extension still requires target validation.
Credentials, cookies, and browser session material remain outside the Salix protocol.

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

The bridge now has both the content-free semantic Conversation probe and the validated
text-only browser-relay path. The dev candidate adds bounded file transport while
retaining the same probe and security boundary. The browser relay currently waits for a
completed rendered assistant response before returning it to the P4; true generation-time
streaming is a later tranche. On the native side, completed response events are drained as one available
batch and coalesced into one Conversation presentation update, avoiding artificial
per-delta pacing on the P4.

Credentials and browser session state remain outside the bridge contract. Normal
LibreWolf owns those details on the modern machine.
