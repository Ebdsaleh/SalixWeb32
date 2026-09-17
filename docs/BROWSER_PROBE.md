# Browser Probe v1

Browser Probe is a diagnostic workspace for discovering what a modern URL actually returns before SalixWeb32 commits to a browser/runtime strategy.

It is deliberately **not a browser renderer**. SalixWeb32 keeps native presentation ownership while the probe answers narrower questions:

```text
URL
 -> modern HTTPS fetch
 -> status / redirects / MIME / headers
 -> raw response body
 -> lightweight extracted text / HTML signals
 -> SalixWeb32 diagnostic presentation
```

The immediate goal is to inspect `https://www.chatgpt.com/` and other targets from the real Pentium 4 without loading a full Chromium-class renderer on that machine.

## Architecture

The first implementation reuses the already validated remote bridge path:

```text
SalixWeb32 Browser Probe
        |
        v
WebPlatformHost
        |
        v
RemoteBridgeWebBackend
        |
        v
Win32HttpTransport / Winsock2
        |
        v
trusted LAN
        |
        v
salix_bridge.py on the modern companion
        |
        v
Python urllib / modern TLS
        |
        v
http:// or https:// target
```

The Browser Probe UI does not know how the fetch is implemented. A future native NT5 HTTPS provider can populate the same backend-neutral snapshot fields without changing the workspace.

## Browser workspace

The existing web workspace is evolved into the Browser Probe rather than adding a second competing web concept.

It contains:

- an address field,
- `Go`,
- backend/capability/status diagnostics,
- `Summary`, `Headers`, `Raw`, and `Extracted` views.

The address field starts with:

```text
https://www.chatgpt.com/
```

A remote fetch is **not** performed during application startup. Pressing `Go` is explicit so a slow or unreachable Internet target cannot unexpectedly stall startup.

### Summary

Shows the useful first-pass facts:

```text
requested URL
final URL after redirects
HTTP status
MIME type
captured response bytes
redirect count
script count
form count
link count
```

### Headers

Shows response headers returned by the upstream target. Sensitive response headers such as `Set-Cookie` are redacted by the companion before the plaintext LAN response is created.

### Raw

Shows the captured textual response body. The companion captures at most 512 KiB in this tranche; the on-screen label intentionally displays at most 32 KiB so a large page cannot turn the diagnostic view itself into a memory/performance test on the Pentium 4.

### Extracted

For HTML, the companion performs deliberately lightweight text extraction and counts a few useful document signals. It does **not** claim to be a DOM implementation. Script/style/noscript/SVG contents are skipped for the text view.

For text and JSON responses, the decoded response can be exposed directly as extracted content.

## Starting the companion

On the modern companion machine:

```text
python tools/salix_bridge.py --host 0.0.0.0 --port 8765
```

Expected startup output now includes:

```text
Salix bridge protocol : SALIX-BRIDGE/1
Browser probe protocol: SALIX-PROBE/1
Modern HTTPS probe    : enabled (unauthenticated GET only)
Credentials/cookies   : never forwarded by Browser Probe
```

Keep the existing host firewall rule narrowly scoped to the Pentium 4 source address.

## Starting SalixWeb32

From the configured Pentium 4 command prompt:

```text
set SALIX_WEB_BACKEND=remote
set SALIX_BRIDGE_HOST=<modern-machine-LAN-IP>
set SALIX_BRIDGE_PORT=8765
bin\Debug\SalixWeb32.exe
```

Open the web/browser workspace, leave `https://www.chatgpt.com/` in the address field for the first test, and press `Go`.

## Security boundary

Browser Probe v1 is intentionally **unauthenticated**.

It does not:

- forward cookies from the modern companion,
- forward browser sessions,
- send authorization headers,
- accept credentials from SalixWeb32,
- log into ChatGPT,
- execute target JavaScript,
- upload files.

Only absolute `http://` and `https://` targets are accepted.

The P4-to-companion hop is still plaintext HTTP and is therefore suitable only for the existing trusted, narrowly firewalled LAN development setup. Do not put credentials, tokens, session cookies, or private conversation data through Browser Probe v1.

## Current limits

The first probe deliberately keeps the implementation small:

- explicit synchronous `Go` request,
- upstream fetch timeout: 10 seconds,
- P4 bridge receive timeout: 12 seconds in remote mode,
- maximum captured upstream body: 512 KiB,
- maximum extracted text: 64 KiB,
- maximum on-screen probe section: 32 KiB,
- no JavaScript execution,
- no DOM,
- no authenticated session,
- no browser rendering,
- no file upload,
- no automatic background polling.

The synchronous request is acceptable for this diagnostic tranche because it makes failures obvious and bounded. A later async request/task boundary can be added if the probe becomes a long-lived interactive tool.

## What this tells us

The probe exists to replace speculation with measurements.

For a target such as ChatGPT we can determine, on the real hardware:

```text
What does the first HTTPS response contain?
How many redirects happen?
What MIME type arrives?
How large is the bootstrap document?
How script-heavy is it?
What useful text/data exists before JavaScript runs?
What information is absent until a runtime executes the application?
```

Those observations drive the next runtime decision. They do not commit SalixWeb32 to Chromium, Gecko, WebKit, a native engine, or the remote bridge.

## Target validation checklist

1. Pull the tranche and reopen VS.NET 2003.
2. Clean/rebuild Debug Win32 with VC7.1.
3. Confirm zero compiler/linker errors; record warnings separately.
4. Start the updated companion on the Server 2022 machine.
5. Launch SalixWeb32 with the remote backend environment variables.
6. Confirm the application reaches the normal shell without automatically fetching the Internet target.
7. Open the web/browser workspace and confirm the URL field contains `https://www.chatgpt.com/`.
8. Press `Go`.
9. Confirm the companion logs `POST /v1/fetch` from the P4.
10. Confirm Summary reports a real upstream HTTP status, final URL, MIME type, byte count, and HTML signal counts.
11. Confirm Headers contains upstream response headers with `Set-Cookie` redacted if present.
12. Confirm Raw contains the beginning of the real upstream response body.
13. Confirm Extracted contains lightweight text when the response is extractable.
14. Probe a simple known text/HTML URL as a control and compare results.
15. Switch Conversation -> web/browser -> Runtime repeatedly and confirm the existing tab/native-control lifecycle remains stable.
16. Close SalixWeb32 and confirm clean shutdown.

Only the real P4 build/runtime test should mark Browser Probe v1 target-validated.
