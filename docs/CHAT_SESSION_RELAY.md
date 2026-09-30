# LibreWolf Chat Session Relay

This document describes the first real ChatGPT text-in/text-out baseline for SalixWeb32.

## Goal

The user should be able to type in the native SalixWeb32 Conversation interface on the
Pentium 4 and receive the real ChatGPT response back in the native Salix conversation
view.

The modern machine is temporary development scaffolding. It hosts the current browser
and service session so the required behavior can be proven before equivalent NT5-native
capabilities exist.

## Process split

```text
Pentium 4 / Server 2003
    SalixWeb32.exe
        |
        | SALIX-CONVERSATION/1 over trusted development LAN
        v
Modern companion
    salix_bridge.py :8765
        |
        | localhost JSON only
        v
    salix_chat_session.py :8766
        ^
        |
        | localhost fetch
        |
    SalixWeb32 Chat Relay WebExtension
        |
        v
    normal LibreWolf
        |
        v
    chatgpt.com
```

`salix_bridge.py` remains the only P4-facing listener.

`salix_chat_session.py` is localhost-only and no longer launches or controls the
browser. It is a broker between the bridge and a small LibreWolf WebExtension.

LibreWolf itself runs normally with the user's normal installed profile. There is no
Selenium, GeckoDriver, Marionette, WebDriver automation mode, or secondary Salix browser
profile in the normal workflow.

## Authentication

Authentication stays entirely inside ordinary LibreWolf.

The user logs into ChatGPT normally using their regular browser profile. SalixWeb32,
`salix_bridge.py`, `salix_chat_session.py`, and the relay protocol do not receive or
store:

- account passwords,
- MFA values,
- browser cookies,
- authorization headers,
- local/session storage.

The extension only exchanges relay readiness, message text, and rendered assistant
response text with the localhost broker.

## First baseline scope

Enabled in the current dev candidate:

- current open ChatGPT thread,
- Salix user text -> normal LibreWolf ChatGPT composer,
- bounded Salix file attachments -> normal LibreWolf ChatGPT composer,
- rendered assistant text -> Salix semantic events,
- bounded assistant-returned files -> Salix semantic attachment events,
- repeated requests in the same browser session.

Not enabled yet:

- thread picker/new-thread control from Salix,
- unrestricted/large file transfer,
- credentials/session transfer,
- generation-time byte streaming.

The current browser relay waits for the rendered assistant response to stabilize. The
bridge then divides completed text into bounded `text_delta` events and appends semantic
`attachment` events for any returned files captured from the assistant turn. Salix keeps
those semantics intact, drains every event already available from the completed response,
and coalesces text presentation while storing returned files under the application data
root. Future true streaming can still present once per newly arrived batch.

## File-relay candidate

WebExtension `0.2.9` is the validated companion baseline for bounded text/generic/image relay on the Server 2003 target.

Outgoing Salix files are transferred as bounded attachment descriptors through the
trusted development LAN, localhost broker, and extension. The content script reconstructs
browser `File` objects and supplies them to the visible ChatGPT composer using the
page's file input or a drag/drop-compatible fallback.

Returned assistant files are discovered from the completed assistant turn, downloaded
inside the authenticated browser context, bounded, and returned as semantic attachment
payloads. Browser credentials/cookies are used only by the browser itself to perform
normal authenticated fetches; they are not copied into the Salix protocol.

First-pass limits:

```text
8 files maximum
2 MB maximum per file
4 MB maximum total file bytes
```

The temporary extension must be reloaded after pulling this candidate because its
manifest/background version is now `0.2.7`. Version `0.2.1` validated the final
ChatGPT composer submission after attachment upload. Version `0.2.2` added browser-owned
capture of returned ChatGPT `sandbox:` file links. Version `0.2.3` broadened discovery
and proved that the returned file UI could be reached, but it entered LibreWolf's
interactive Save As flow. Version `0.2.4` still triggered the interactive Save As flow because it clicked the
page's Download control first. Version `0.2.5` instead extracts the HTTP(S) URL exposed
by the preview Download control and starts a background `saveAs:false` download directly,
without clicking the Download UI or supplying a filename.

The first `0.2.5` retest proved that the explicit Download control exposes no usable URL
in the rendered DOM. Version `0.2.6` therefore uses a temporary blocking `webRequest`
listener during returned-file capture: it allows the JavaScript Download action to reveal
the real signed file-content request, cancels that request before interactive Save As,
and replays the captured URL as the managed `saveAs:false` download.

The `0.2.6` retest validated that interception strategy end-to-end: returned file bytes
reached the P4 and caused the native `Received` directory to be created automatically.
The remaining issue was duplicate/generic metadata (`content`, `content(1)`) because
two controls represented one returned file and the signed endpoint filename was generic.
Version `0.2.7` preserves the semantic assistant filename, deduplicates those controls,
and restores MIME inference from that name.

The `0.2.7` retest is green: the extension collected one attachment from two candidate
controls, skipped the duplicate candidate, the bridge returned one attachment, and the
P4 stored one correctly named `.txt` file that opened normally in Notepad. This closes
the basic bidirectional text-file relay validation on the Server 2003 target.

A later single-PNG send failed before assistant response capture. The localhost broker
reported `ChatGPT Send control did not accept the relay submission`; this was then
surfaced to the P4 as the bridge's generic HTTP 503 browser-relay failure. Version
`0.2.8` extends only the filename-less image-upload settle period before automatic Send.
It does not change the validated returned-file interception path.

The subsequent `0.2.9` target retest succeeded. The extension submitted the single PNG
after the MIME-aware settle window, returned one assistant attachment, suppressed the
duplicate browser candidate, and completed `POST /v1/message` successfully. The bridge
then returned one attachment with HTTP 200 to the P4.

Target-side follow-up video validation confirmed the returned PNG's native Salix Preview
and external Open paths. The ordinary PNG file relay is now fully validated end-to-end on
the Server 2003 target.

The `0.2.8` retest exposed that visible image filenames still took the old immediate
named-file path, so its settle change could be bypassed. Version `0.2.9` keys the settle
decision from attachment MIME type instead: `image/*` always waits through the image
settle window. Submit verification is also extended for attachment requests, generation
activity is accepted as submit evidence, and one `form.requestSubmit()` fallback plus
diagnostic submit-state snapshot is available if the normal click does not take.

The modern smoke helper can now exercise an outgoing file without the P4:

```bat
python tools\test_chat_relay.py --message "Attachment smoke test" --file path\to\small.txt
```

The helper uses the same bounded `SALIX-CONVERSATION/1` framing as the native backend
and prints any returned semantic attachment events.

The `0.2.1` modern smoke pass is validated: a small text file was attached through the
helper, appeared in the visible ChatGPT composer, and was submitted automatically without
a manual Send click. The file arrived successfully in the ChatGPT conversation.

## Modern-machine setup

Start LibreWolf **normally** using the profile you already use for ChatGPT.

Then run:

```bat
tools\setup_chat_session.bat
```

The helper opens LibreWolf's development-extension page when LibreWolf is installed in a
standard Program Files location and prints the exact extension manifest path.

In LibreWolf:

1. open `about:debugging#/runtime/this-firefox`,
2. click **Load Temporary Add-on...**,
3. select:

   ```text
   tools\librewolf_chat_relay_extension\manifest.json
   ```

4. keep LibreWolf running,
5. open the ChatGPT conversation you want Salix to use.

Firefox-family browsers support loading a development WebExtension this way. The
temporary extension remains installed until LibreWolf restarts.

Now start the localhost broker:

```bat
python tools\salix_chat_session.py
```

Expected startup includes:

```text
Browser control       : normal LibreWolf WebExtension (no Marionette)
Authentication        : existing normal LibreWolf profile/session
```

Then start the P4-facing bridge in another terminal:

```bat
python tools\salix_bridge.py --host 0.0.0.0 --port 8765
```

Keep the existing firewall rule restricted to the P4. Port 8766 remains localhost-only
and should not be exposed to the LAN.

## Modern-side smoke test

Before involving the P4:

```bat
python tools\test_chat_relay.py
```

When the WebExtension is loaded and the ChatGPT composer is visible, bridge health should
contain:

```text
conversation_browser_session=ready
```

Then run:

```bat
python tools\test_chat_relay.py --message "Hello from the Salix relay smoke test"
```

The message should appear in the currently open ChatGPT conversation in normal
LibreWolf. After the assistant response stabilizes, the returned text should be printed
in the terminal through the same semantic framing used by the P4.

## Relay health contract

`salix_chat_session.py` considers the browser side ready only when:

- the WebExtension has sent a recent heartbeat,
- a ChatGPT tab is open,
- the content script can see the ChatGPT composer.

If the extension is not loaded, health reports:

```text
extension_not_connected
```

If the extension is loaded but no usable ChatGPT composer is visible:

```text
chatgpt_composer_not_ready
```

When ready:

```text
ready
```

The bridge exposes that as:

```text
conversation_browser_session=ready
```

## Browser interaction strategy

The extension uses a background script for localhost communication and a content script
for page interaction.

The content script prefers semantic browser/page attributes rather than screen
coordinates.

Composer lookup includes:

- ChatGPT prompt textarea forms,
- `#prompt-textarea`,
- ChatGPT contenteditable forms.

Submission prefers the page's enabled Send control and falls back to an Enter key event.

Assistant extraction prefers `data-message-author-role="assistant"` and then a
conversation-turn/Markdown fallback. Decorative controls are removed from the copied
assistant text.

These selectors are isolated inside the extension so normal ChatGPT markup changes do
not require changes to the VC7.1 application.

## WebExtension 0.3.5 rendered-thread response fallback candidate

Real-P4 testing established two separate facts:

1. the validated WebExtension 0.2.9 browser relay returned ordinary completed responses
   correctly,
2. its synchronous lifetime could fail around 180 seconds while ChatGPT was still
   working.

Experimental 0.3.0-0.3.2 changes attempted to redesign browser completion detection while
also solving the timeout. Target testing showed that those changes regressed the known-good
return path: the browser visibly completed an answer but the extension did not emit
`/v1/result`, leaving the P4 at `request in flight`.

Those experimental completion changes have been removed from the active candidate.

Historical validation identifies WebExtension 0.1.0 at commit `9321882` as the first
successful P4 text-return baseline, and 0.2.9 later returned completed responses during
the validated PNG round trip. The active 0.3.5 candidate keeps the validated 0.2.9
response lifecycle and extended wait ceilings. In addition to current-DOM assistant
discovery, it adds a selector-independent fallback: after the exact submitted user
message is rendered, the extension anchors to that message, snapshots the static thread
tail, and treats only newly rendered thread text after that anchor as the assistant
response candidate.

The active synchronous safety chain is:

```text
WebExtension response wait : 4 hours
chat-session broker        : 4 hours + 1 minute
bridge worker              : 4 hours + 2 minutes
native Conversation HTTP   : 4 hours + 3 minutes
```

No turn-correlation rules, completion-action heuristics, heartbeat failure semantics, or
follow-up behavior are part of 0.3.4.

This tranche answers two tightly scoped questions: can the proven completed-response path
discover the current rendered assistant turn again, and can it remain alive beyond the
historical 180-second ceiling?


## Planned live browser-workflow telemetry

The current relay treats the browser as a request/response endpoint and uses a fixed
response deadline. The next planned tranche narrows the behavioral distance between the
native Salix composer and the visible browser by exposing provider workflow state while a
request is active.

The extension will report two independent browser-facing states in addition to transport
receipt:

```text
provider generation:
    idle
    generating
    stabilizing
    completed
    cancelled
    provider_failed
    connection_lost

provider composer:
    unavailable
    ready
    submitting
    followup_ready
```

The browser adapter should infer these states from several live DOM/accessibility signals
rather than one literal selector or one piece of placeholder text. Relevant observations
include:

- whether the supported ChatGPT composer is visible and editable,
- whether the composer is empty after submission,
- whether a Send or Stop-generation control is available,
- whether generation controls indicate active work,
- whether the composer placeholder/ARIA state suggests a follow-up,
- whether the submitted user turn appeared,
- whether the assistant turn is still changing,
- whether provider error/retry UI is visible,
- whether the user explicitly stopped the active browser generation.

The extension already has generation-control detection; the planned tranche turns that
kind of observation into explicit telemetry instead of using it only as an internal
completion helper.

Pixel or canvas inspection is not the default strategy when semantic DOM/accessibility
state is available. If a future provider exposes a critical state only through rendered
pixels, that belongs in a provider-specific visual adapter below the same semantic
contract.

### Follow-up behavior

ChatGPT can expose a usable composer while the current assistant generation is still
active. This must not be mistaken for completion.

The extension will report:

```text
generation = generating
composer   = followup_ready
```

Salix may then re-enable its native input immediately. The next ordinary Salix message is
sent through the same text + bounded-attachment pipeline as any other message; no special
"follow-up" command is required.

Each follow-up remains its own immutable Salix request with its own request ID and receipt
verification. The provider adapter may associate several accepted requests with one
active generation/context group until the provider finishes or is cancelled.

### Planned receipt acknowledgement

Before waiting for provider generation, the P4 and bridge will establish that the complete
request reached the companion. Planned receipt metadata includes:

```text
request_id
payload_bytes
payload_sha256
verified
```

The digest is SHA-256 over the canonical request payload. This proves complete bridge
receipt; provider acceptance is reported separately after the browser UI confirms that
the submitted user turn was accepted.

### Planned liveness sources

The relay will distinguish:

- P4 -> bridge health,
- companion outbound Internet/provider reachability,
- WebExtension heartbeat,
- ChatGPT tab/composer readiness,
- provider generation state,
- provider composer state.

These checks run outside the native UI thread. ICMP ping may be exposed as optional
diagnostic information but is not the authoritative health check.

Live status reports are planned to carry monotonic sequence numbers so delayed packets
cannot regress the native client from a newer state to an older one.

### Planned timeout semantics

A long-running model response is not itself a failure. The planned policy keeps a bounded
submission-acceptance deadline but removes the present short fixed generation deadline as
the normal failure condition.

While the bridge, extension, and provider workflow remain observably alive, Salix should
continue waiting whether generation takes seconds or many minutes.

Explicit browser cancellation, provider error UI, extension loss, bridge loss, and
Internet/provider reachability loss become separate semantic outcomes. HTTP 503 should
represent genuine companion/service unavailability rather than merely a long-running
generation.

## Development-extension lifecycle

The initial relay uses a **temporary** WebExtension installation to prove the architecture
without introducing signing/distribution work into the baseline.

After LibreWolf restarts, reload the extension from:

```text
about:debugging#/runtime/this-firefox
```

Once the relay is target-green, persistent packaging/signing or another deployment method
can be handled as a separate tranche.

## Failure behavior

If the extension is missing, the localhost broker remains healthy but reports
`extension_not_connected`.

If no ChatGPT composer is available, the broker reports
`chatgpt_composer_not_ready`.

If page interaction fails after a request begins, the extension returns a failure to the
localhost broker, which propagates through `salix_bridge.py` into the existing
Conversation failure event path.

The validated 0.2.9 implementation had a 180-second WebExtension/session response
deadline. Real target evidence showed P4 requests failing after approximately 181.5-181.7
seconds, consistent with that synchronous deadline. This is tracked as a relay-liveness
limitation, not as a ConversationView character or line limit. The active 0.3.3 candidate
keeps the validated completion detector unchanged and extends only the synchronous wait
ceilings. The planned stateful liveness design above remains the longer-term replacement
for synchronous request/response waiting.

The ordinary LibreWolf window remains visible throughout, so browser-side failures can be
inspected directly.

## Relay timing instrumentation

The validated relay now carries diagnostic-only timing metadata through the existing
path without changing dispatch/security behavior.

The WebExtension measures:

```text
browser_submit_ms
browser_first_response_ms
browser_generation_ms
browser_stabilization_ms
browser_total_ms
background_total_ms
```

The localhost broker adds:

```text
broker_queue_ms
broker_extension_ms
broker_total_ms
```

The bridge adds:

```text
bridge_worker_ms
bridge_total_ms
```

These values are returned as `timing_...` metadata in the existing
`SALIX-CONVERSATION/1` response header. They are diagnostic metadata only; the
semantic event sequence and message body framing are unchanged.

On the P4, SalixWeb32 adds a local `P4 total` measured with `GetTickCount()` from
accepted native submission until `message_completed` is consumed. This is deliberately
a user-visible total and therefore includes LAN/HTTP receive, semantic event draining,
and native presentation overhead after the modern-side relay completes.

`Options -> Debug -> Copy Diagnostic Report` exposes the combined result under:

```text
Conversation Relay Timing
-------------------------
P4 total ... ms | Relay timing: bridge ... | broker ... | ...
```

The modern smoke helper also prints every timing field before the semantic events:

```bat
python tools\test_chat_relay.py --message "Salix timing smoke test"
```

Timing instrumentation completed modern-side and real-P4 validation on
September 19, 2026. The real target measurement was:

```text
P4 total 57484 ms
bridge 19394 ms
broker 19386 ms
queue 16 ms
extension 19362 ms
browser 19367 ms
submit 117 ms
first response 350 ms
generation 16766 ms
stabilize 2134 ms
```

The same P4 candidate rebuilt cleanly under VC7.1 with zero errors and zero warnings.
That first measurement exposed an artificial one-event-per-update throttle in the native
remote Conversation backend. The corrected target retest measured:

```text
P4 total 7406 ms
bridge 7222 ms
native batch 27 deltas -> 1 updates
present 0 ms
```

The corrected path therefore reduced the bridge-to-native completion gap to about
184 ms and removed the visible post-response drip feed.

## Target validation

The baseline was validated end-to-end on the real Pentium 4 / Windows Server 2003 target
on September 19, 2026.

A native SalixWeb32 message reached the currently open authenticated ChatGPT thread
through the bridge, localhost broker, and normal-LibreWolf WebExtension. The real
assistant response returned through `SALIX-CONVERSATION/1` semantic events and rendered
inside the native Conversation view. The VC7.1 target build was clean with zero errors
and zero warnings.

The validated baseline remains completed-response-oriented. Bounded ordinary
text/generic/image file relay is now target-green in both directions, while the active
native candidate adds composer-side attachment preflight UX. Browser-side
generation/stabilization latency and long-running request liveness remain follow-up
targets, while native completed-response batching is already validated on the real P4.
True generation-time streaming remains a separate later tranche.

### September 30, 2026 relay-recovery validation

WebExtension `0.3.15` is now target-green on the real Pentium 4 / Windows Server 2003
path for ordinary text response recovery plus assistant-returned image relay.

The recovery work preserved the validated 0.2.9 request/response lifecycle while adapting
to current ChatGPT rendered markup. Real-target evidence established two current-DOM
failure modes that were not native transport failures:

- short/transient rendered states such as `Thinking` could be mistaken for the completed
  assistant response,
- returned-file card text can flatten the semantic filename directly against provider UI
  chrome, for example `SalixWeb32-remote-test-image.pngImageOpen file`.

The 0.3.15 candidate keeps the rendered-response fallback/guarding work from the recovery
line and recognizes the flattened attachment-card suffix without discarding the real
semantic filename.

The validated P4 run reported:

```text
P4 total 12157 ms
bridge 11482 ms
broker 11462 ms
queue 179 ms
extension 11272 ms
browser 7750 ms
submit 850 ms
first response 2633 ms
generation 2183 ms
stabilize 2084 ms
native batch 23 deltas -> 1 updates
present 0 ms
files 1
```

The assistant-returned PNG reached the Pentium 4, rendered inline in the native
Conversation view, and was then sent back for integrity comparison. The returned payload
was byte-for-byte identical to the source:

```text
size:    151901 bytes
SHA-256: 3d7fb733123e8fa430cc076e69b578bf6acd830912cd74061b72fe2c2fb2115b
```

This validates the current recovery branch as an end-to-end text + returned-image
baseline without requiring a new P4 executable build.

The UTF-8 framework / UTF-16 Win32 boundary and glyph-aware fallback are also validated
on the real P4, including Unicode clipboard and relay round-trip coverage.

See `docs/VALIDATION.md` for the full evidence and regression checklist.

## WebExtension 0.4.0 superseding follow-up validation

Real Pentium 4 testing of the 0.3.16 recovery line exposed a separate architectural
limitation after provider-progress filtering was corrected: one browser-relay request
could remain open while ChatGPT continued working, and every later Salix Send was rejected
as `SALIX-CONVERSATION/1 browser relay request in flight`.

That behavior came from three independent single-flight assumptions:

- the native `RemoteConversationBackend` owned one active request/executor lane,
- `salix_chat_session.py` owned one `PendingRequest`,
- the WebExtension background worker kept `commandBusy` until the complete assistant
  response returned.

The 0.4.0 candidate introduces **superseding follow-up semantics**. It does not attempt
to run two assistant generations concurrently in one ChatGPT thread.

When the user sends a newer Salix message while an older browser turn is still open:

1. the native backend may dispatch the newer request through a second independent Win32
   conversation transport/executor lane;
2. the localhost broker marks the older pending request as superseded and wakes its
   waiting HTTP worker;
3. the WebExtension background poller may dispatch the newer command while the older
   content-script promise is still alive;
4. the content script invalidates the older submission token, attempts to stop any visible
   provider generation control, and submits the newer follow-up;
5. stale results/events belonging to the older request are rejected instead of reclaiming
   native presentation ownership.

The intended user-visible result is that a stuck provider turn no longer holds the native
composer hostage. A new follow-up becomes authoritative and the older request resolves as
interrupted/superseded.

This tranche changes native C++ and therefore **requires a P4 rebuild**. It remains a
target-validation candidate until the real Server 2003/Pentium 4 test proves:

- request 1 can remain in flight,
- request 2 is accepted from the native composer,
- request 1 is reported/ignored as superseded,
- request 2 visibly reaches the active ChatGPT thread,
- only request 2's assistant response is rendered back into Salix,
- normal returned-file handling still works after the supersede path.
### Real Pentium 4 validation — September 30, 2026

The real Pentium 4 / Windows Server 2003 target validated the first superseding
follow-up path end-to-end.

Observed sequence:

- request 1 entered the relay with 49 text bytes and two attachments;
- request 2 entered while request 1 was still active;
- the broker marked request 1 superseded by follow-up request 2;
- request 1's waiting HTTP path returned and the P4 rendered the intentional
  `Previous response interrupted by newer follow-up.` system event;
- the WebExtension trace for request 2 reported `followup=yes`;
- request 2 submitted into the live ChatGPT thread, produced 608 response bytes,
  completed `POST /v1/result` with HTTP 200, and returned through the bridge with
  HTTP 200;
- the native client returned to `SALIX-CONVERSATION/1 ready` after completion.

Measured request-2 target timing:

```text
P4 total 30531 ms
bridge 30095 ms
broker 30078 ms
queue 347 ms
extension 29723 ms
browser 29684 ms
submit 10434 ms
first response 10784 ms
generation 6333 ms
stabilize 2133 ms
native batch 28 deltas -> 1 updates
present 0 ms
files 0
```

This validates one newer native user turn superseding one older in-flight browser
relay turn. It does **not** yet claim unlimited rapid follow-up depth or concurrent
assistant generations. Returned-file behavior after a supersede remains a separate
follow-up validation target.

## Unified chat server candidate

The `relay-unified-0.5.0` branch introduces `tools/chat_server.py` as the normal
modern-side entry point.

Usage stays compatible with the historical bridge command:

```bat
python tools\chat_server.py --host 0.0.0.0 --port 8765
```

One Python process now owns:

- the P4-facing trusted-LAN bridge on `--host` / `--port`,
- the LibreWolf WebExtension endpoint on `127.0.0.1:8766`,
- one shared `RelayState` for request ownership, supersede state, timing, and diagnostics.

Conversation requests received on the LAN listener call the shared relay state directly
in-process. The previous bridge-to-broker localhost HTTP hop is removed from the normal
path. The localhost HTTP endpoint remains because the browser WebExtension still requires
a browser-accessible endpoint.

`salix_bridge.py` and `salix_chat_session.py` remain available as standalone
diagnostic/fallback entry points during migration.

