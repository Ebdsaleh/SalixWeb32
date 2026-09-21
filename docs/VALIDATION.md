# SalixWeb32 Target Validation

This document records validated legacy-target baselines and the major defects found while validating them. Active work after `v0.0.2` is tracked in `docs/POST_V002_VALIDATION.md` so experimental composer/conversation tranches can advance without rewriting the tagged-baseline history.

## Target environments

Primary target:

```text
CPU:        Pentium 4 class
Memory:     2 GB
OS:         Windows Server 2003 SP2 x86
Compiler:   Visual C++ 7.1 / Visual Studio .NET 2003
Executable: SalixWeb32.exe
```

Deferred compatibility target:

```text
MiniXP on the same Pentium 4 hardware
```

MiniXP remains historical compatibility evidence only for now. Active tranche acceptance
is performed on Windows Server 2003 R2 / Pentium 4. MiniXP testing is deferred until the
Server 2003 feature set is complete and the MiniXP environment is usable again; current
MiniXP failures include a `gdiplus.dll` problem and a generally incomplete stripped
environment. Previous MiniXP passes remain valid historical records but do not create an
active validation requirement.

## Phase 1 native skeleton baseline

Validated commit:

```text
4628025 Add Phase 1 native Win32 runtime skeleton
```

Validated on Windows Server 2003:

- VS.NET 2003 solution opens successfully,
- Debug configuration compiles and links with VC7.1,
- native application launches from Visual Studio and Command Prompt,
- `ApplicationRuntime` initializes,
- `Win32ApplicationHost` creates the application window,
- message loop remains responsive,
- status text renders,
- normal close exits cleanly.

The same executable also launched successfully under MiniXP.

## Phase 1 hardening

Relevant commits:

```text
5547277 Harden Phase 1 runtime lifecycle and resize handling
0fd362e Fix Win32 status repaint ghosting
```

Validated behavior:

- `RuntimeStatusService` starts through `ServiceRegistry`,
- runtime ticks advance continuously,
- service count reports correctly,
- client-area dimensions update during resize,
- normal shutdown remains clean.

Target validation exposed repaint ghosting because live status text was redrawn over stale pixels with a transparent text background. Clearing the client area inside the paint path corrected the defect. The corrected build was revalidated with repeated resizing and sustained runtime updates.

The validated Phase 1 baseline is tagged:

```text
v0.0.1
```

## Phase 2 presentation separation

Validated commit:

```text
c3a3406 Begin Phase 2 framework presentation separation
```

Validated on the target:

- framework `Label` components represent status text,
- `StatusView` renders through the backend-neutral `View` contract,
- Win32-specific drawing remains behind `Win32ComponentRenderer`,
- live status and resize behavior remain correct,
- repaint ghosting does not regress.

## Phase 2 container/layout

Validated commit:

```text
01892b6 Add container and stack layout framework
```

Validated behavior:

- VC7.1 accepts `Container` and `StackPanel`,
- status presentation renders through the child hierarchy,
- vertical stack layout reproduces the previous presentation,
- live updates, resize, and shutdown remain functional.

## Phase 2 interactive controls

Relevant commits:

```text
e574395 Add interactive framework controls and event dispatch
93b54f0 Fixed missing 'UIEvent.h' include directive
```

The first rebuild exposed a compile error because `Component.cpp` used `UIEvent` without including its definition. The missing include was added from the target machine. The corrected build compiled, linked, and ran successfully.

Validated behavior:

- backend-neutral `Button` pointer interaction,
- backend-neutral `TextInput` focus/text entry,
- printable character input and Backspace,
- Win32-to-framework event dispatch,
- callbacks and styling primitives,
- continued runtime updates and clean resize/shutdown behavior.

## Messenger layout and double buffering

The messenger shell and `MessageInputStrip` were exercised on both Windows Server 2003 and MiniXP.

Validated behavior included:

- expanding text field with right-anchored Send button,
- Enter-to-submit and button-click submission,
- composer clear after send,
- responsive diagnostics sidebar.

MiniXP exposed full-window flashing during periodic runtime repaints. The fix:

```text
dd880eb Double-buffer Win32 painting to prevent flicker
```

changed the Win32 host to suppress the separate erase pass, render the complete application into a compatible memory DC/bitmap, and `BitBlt` the completed frame to the real DC. The corrected build was revalidated successfully on both target environments with no observed periodic flashing.

Dirty-region/component invalidation remains a later rendering improvement.

## Text cursor navigation

Validated commit:

```text
7273f18 Add cursor navigation to text input
```

Validated behavior:

- Left/Right movement,
- Home/End,
- Delete and Backspace,
- insertion at the caret,
- visual caret at the actual cursor position,
- Enter-to-submit after in-place editing.

## Selection, clipboard, and advanced editing baseline

Relevant commits:

```text
b38835b Add text selection clipboard and MIME input foundation
3247a81 Add word navigation and selectable read-only labels
7321067 Add multi-click text selection
ad147f1 Add discontinuous text selection and paste modes
0cc832e Add subtractive text selection gestures
20e0092 Add cut modes and text undo redo
```

Validated across Windows Server 2003 SP2 and MiniXP:

- mouse selection in framework text controls,
- selectable read-only conversation text,
- clipboard copy from read-only labels,
- Ctrl+Arrow / Ctrl+Shift+Arrow word navigation,
- double-click word and triple-click line selection,
- discontinuous/additive selections,
- Ctrl+Alt subtractive selection workflows,
- compact and preserved-spacing clipboard representations,
- compact and keep-formatting paste modes,
- compact and keep-formatting cut modes,
- bounded Undo/Redo,
- restoration of text/caret/discontinuous selection state,
- external clipboard interoperability.

Interaction detail recorded during validation: a preserved highlighted range can remain selected while Ctrl+click relocates the caret. Additional regions can be added while retaining the existing highlight through the established additive selection gesture (including Shift+drag where appropriate). This behavior is intentional and is part of the documented selection model.

The final advanced editing baseline is tagged:

```text
v0.0.2
```

## Append-only conversation history

The previous single-message conversation label was replaced by an append-only `ConversationView`. Historical messages remain present and independently selectable instead of being overwritten.

This became the foundation for later native conversation scrolling, Markdown presentation, wrapping, and block-composed messages.

## Post-v0.0.2 validation

Post-baseline work now includes:

- toolbar/composer formatting,
- graphical classic emoticons,
- multiline/list editing,
- code composer mode,
- Markdown presentation,
- native composer/conversation scrollbars,
- mouse-wheel input,
- wrapped pixel-scrolled conversation history,
- code-vs-normal semantic presentation,
- the current dedicated code-block container work.

These newer tranches do **not** inherit the `v0.0.2` validation status automatically.

See:

```text
docs/POST_V002_VALIDATION.md
```

for the current Server 2003 observations, compatibility fixes, remaining MiniXP coverage, and the target checklist for the active block-composed code presentation tranche.


## Phase 3 remote bridge and Browser Probe validation

### Remote transport foundation

The remote bridge transport has been exercised between the real Pentium 4 /
Windows Server 2003 SP2 target and a Windows Server 2022 companion.

Validated observations include:

- P4 client address reached the companion over the trusted LAN,
- companion health endpoint responded,
- bounded failure occurred when the bridge port was blocked,
- the narrowly scoped firewall rule restored the connection,
- `POST /v1/navigate` completed successfully,
- SalixWeb32 remained stable through bridge failure/retry conditions,
- the remote backend stayed behind the generic `WebPlatformBackend` boundary.

### Browser Probe modern HTTPS path

On September 18, 2026, Browser Probe was exercised from the real P4 against:

```text
https://www.chatgpt.com/
```

through the configured Windows Server 2022 companion.

Observed successful result:

```text
Final URL:       https://chatgpt.com/
HTTP:            200 OK
MIME:            text/html
Captured bytes:  497574
Redirects:       1
HTML signals:    scripts 16 | forms 1 | links 11
```

The response contained real ChatGPT HTML rather than a placeholder surface. Headers
arrived with sensitive `Set-Cookie` values redacted by Browser Probe. The extracted
view exposed useful server-rendered text, and the Raw export began with the real
`<!DOCTYPE html>` document.

The complete Raw section was copied out successfully and contained the real HTML body,
confirming that the earlier Headers/Raw ambiguity was no longer present.

### UI/network decoupling

The first Browser Probe implementation performed the blocking bridge request on the UI
thread. Target testing exposed application stalls while the companion performed modern
HTTPS work.

The current implementation moves blocking `NetworkTransport::send()` execution behind
`Win32NetworkRequestExecutor`. Completion is consumed during the ordinary application
update path; the worker does not mutate framework/application UI state.

This materially improved responsiveness during network requests.

### Raw rendering/copy performance

Target testing then isolated a second performance issue unrelated to the bridge:
rendering minified Raw HTML through the current formatted-text path remained expensive
on the Pentium 4, and repaint pressure could make other windows feel visually sluggish.

The current mitigation:

- keeps the complete captured Raw body for Copy,
- limits the on-screen Raw preview to 1 KiB,
- hard-wraps the Raw preview before it reaches the formatted text renderer,
- avoids normalizing/copying the complete Raw body solely for display,
- passes known text lengths through the MIME/Win32 clipboard path rather than repeatedly
  rescanning large buffers with `strlen()`.

The first font-cache tranche was reported **more responsive**. A second renderer tranche
then batched compatible formatted text into runs for GDI measurement/drawing and changed
wrapping to measure useful spans before falling back to character/token-level work.
Real Pentium 4 testing after that tranche was reported **much better performance**.

Raw copy correctness remains confirmed. The bounded 1 KiB Raw preview is intentionally
retained even after renderer improvement: showing hundreds of kilobytes of minified HTML
offers little presentation value, while Copy and diagnostic export preserve access to
the complete cached body.

### Browser diagnostic report export

The one-click Browser report path has now been exercised on the real Server 2003 /
Pentium 4 target.

`Options -> Export Browser Diagnostic Report...` successfully produced a timestamped
text file containing:

- Browser title, URL, backend, capabilities, and HTTP status,
- complete Summary,
- complete redacted Headers,
- complete captured Raw HTML,
- complete Extracted text,
- the final generated report path.

The validated report contained a real ChatGPT HTTP 200 response of 498021 captured
bytes. The complete Raw section remained present in the exported file even though the
on-screen Raw presentation stayed bounded.

This validates the intended presentation-to-interaction pattern: the target does not
pay the rendering cost of the complete payload merely to retain full-data access.

### Persistent remote configuration

The target now uses an ignored machine-local configuration file:

```text
salixweb32.local.ini
```

with the committed `salixweb32.local.ini.example` as a template. This prevents normal
Visual Studio launches from silently falling back to the placeholder backend simply
because per-shell environment variables were not set.

No machine-local address, credential, token, or private session material is committed.

## Current validation boundary

The following should **not** be inferred from the successful Browser Probe pass:

- authenticated ChatGPT/session operation is not implemented,
- the current plaintext LAN bridge is not approved for credentials,
- JavaScript execution is not provided by Browser Probe,
- Browser Probe is not a browser renderer/DOM implementation,
- the large-text path is substantially improved, but future renderer changes still
  require real-target profiling rather than assuming modern-PC behavior,
- MiniXP coverage is not implied by Server 2003 validation unless explicitly recorded.

For subsequent tranches, the real Pentium 4 remains authoritative.


## Semantic conversation-service validation

The provider-neutral Conversation service contract has now been exercised on the real
Windows Server 2003 / Pentium 4 target.

The validated local proof uses `PlaceholderConversationBackend`, which performs no network
access. It emits one semantic event at a time through the same host/backend boundary
intended for future remote/API/web-session providers.

Observed target evidence includes an operational Win32 host, initialized remote web
backend, `Placeholder Conversation Backend | semantic contract ready`, repeated native
Remote messages produced by semantic text-delta events, Markdown presentation, and a
Conversation message count of 9 in the diagnostic report.

Original target checklist:

1. Clean/Rebuild `Debug | Win32` under Visual C++ 7.1.
2. Confirm zero errors and zero warnings.
3. Launch SalixWeb32 and open the Conversation workspace.
4. Confirm the hint reports `Placeholder Conversation Backend` and semantic contract
   ready.
5. Send `Hello from Pentium 4`.
6. Confirm the local message remains in history.
7. Confirm one Remote response appears and is updated from semantic text-delta events.
8. Confirm the bold Markdown heading in the placeholder response renders through the
   existing native Markdown path.
9. Confirm the final response explicitly says that no external conversation service was
   contacted.
10. Send another message after completion and confirm a new request is accepted.
11. Confirm Browser Probe and Runtime continue to work normally.
12. Confirm no credentials, session cookies, conversation content, or attachments are
   transmitted by this placeholder proof.

This target pass validates the application-facing semantic boundary only. It does not
validate authenticated ChatGPT/service access or make the existing plaintext LAN bridge
suitable for private conversation traffic.

### Remote semantic conversation probe — first target pass

The September 19, 2026 target build correctly selected
`Remote Conversation Bridge Backend` whenever the existing bridge configuration
selected remote mode. The diagnostic report recorded an operational runtime/Win32 host,
the initialized Remote Bridge Web Backend, and the Remote Conversation Bridge Backend.

The native Conversation UI remained responsive and local user messages were preserved.
However, both remote Conversation test sends produced:

```text
Remote conversation probe returned HTTP 404.
```

The Browser workspace remained functional in the same run. Its exported diagnostic
report recorded a real ChatGPT `HTTP 200 OK`, one redirect, redacted Set-Cookie headers,
and 524288 captured bytes (truncated at the configured cap). This isolates the observed
failure to Conversation-route/capability compatibility rather than general
P4-to-companion connectivity.

The available target artifacts do not prove whether the Server 2022 machine had an older
checkout or whether a previously started companion process was still running after the
script had been updated. Both states are consistent with the observed HTTP 404.

### Conversation capability handshake — incompatible-companion path validated

The September 19, 2026 target run validated the **negative** capability path. The
diagnostic report showed:

```text
Conversation backend: Remote Conversation Bridge Backend |
companion lacks conversation probe; update/restart companion
```

while the Win32 host and Remote Bridge Web Backend remained operational. That confirms
the client no longer labels the Conversation backend ready merely because its executor
initialized. It queues:

```text
GET /v1/health
```

and requires the companion to advertise:

```text
conversation_probe=enabled
conversation_protocol=SALIX-CONVERSATION/1
```

The remote backend exposes live compatibility state in the Conversation header, including
messages such as:

```text
checking companion conversation capability
SALIX-CONVERSATION/1 ready
companion lacks conversation probe; update/restart companion
conversation protocol mismatch; update/restart companion
```

If an incompatible/unreachable companion is later updated and restarted, a subsequent
send attempt triggers another health check so the application can recover without a full
SalixWeb32 restart.

In the same target session, Browser Probe reached the companion and received an upstream
ChatGPT `HTTP 403 Forbidden` Cloudflare challenge response with redacted Set-Cookie
headers. That is an upstream response, not evidence of local bridge failure, and further
supports that the Conversation warning was capability-specific.

### Conversation capability handshake — positive path validated

The September 19, 2026 target pass closed the positive
`SALIX-CONVERSATION/1` milestone on the real Windows Server 2003 / Pentium 4 target.

Observed evidence:

- the current Server 2022 companion advertised `SALIX-CONVERSATION/1`,
- the P4 completed `GET /v1/health` with HTTP 200,
- the companion accepted two `POST /v1/conversation/probe` requests with HTTP 200,
- the P4 diagnostic reported
  `Remote Conversation Bridge Backend | SALIX-CONVERSATION/1 ready`,
- the same diagnostic reported the Win32 host operational and Remote Bridge Web Backend
  initialized,
- Browser Probe still returned a real ChatGPT `HTTP 200 OK` in the same run.

This validates capability negotiation, Conversation routing, request-ID framing, semantic
event delivery, and coexistence with Browser Probe. The probe still intentionally sends
no draft text, attachment paths, credentials, cookies, or session material.

### Probe-only security-policy gate — validated

The September 19, 2026 follow-up pass validated the explicit probe-only security policy
on the real P4 and current Server 2022 companion.

Observed evidence:

- the companion banner reported `Conversation mode : probe-only`,
- it reported `Sensitive forwarding : attachments/credentials/session disabled`,
- it reported `P4 conversation link : plaintext LAN; real content blocked`,
- the P4 completed `GET /v1/health` with HTTP 200,
- two Conversation requests produced safe audit lines containing
  `text=0 attachments=0 credentials=0 session=0`,
- both `POST /v1/conversation/probe` requests returned HTTP 200,
- the P4 diagnostic reported
  `SALIX-CONVERSATION/1 ready | probe-only | plaintext LAN`,
- Browser Probe remained operational and returned a real ChatGPT HTTP 200 response.

The wire-level privacy gate is therefore target-green.

### Host content-dispatch boundary — validated

The September 19, 2026 follow-up pass also validated the native host-level
`ConversationSecurityProfile` gate on the real P4.

Observed evidence:

- the companion still logged only content-free probes with
  `text=0 attachments=0 credentials=0 session=0`,
- both Conversation probe requests returned HTTP 200,
- the P4 diagnostic reported
  `Conversation security: mode probe-only | transport plaintext | text no | attachments no | credentials no | session no`,
- Browser Probe remained operational and returned a real ChatGPT HTTP 200 response.

This closed the host dispatch-boundary milestone for that probe-only revision. At that
validated historical state, the remote backend could not receive the
`ConversationRequest` object through probe dispatch and its content method was a hard
refusal.

The newer LibreWolf browser-relay tranche deliberately introduces a separate
content/trusted-LAN text-only policy; it does not retroactively change what this earlier
target pass proved.

### Historical secure-provider experiment — superseded

A provider-absent discovery experiment was previously exercised on the P4 and correctly
failed closed. The later plan to build a required `SalixSecureTransport.dll` with a
newer Microsoft toolset was subsequently rejected because it did not match the project's
authoritative P4/VC7.1 build goal.

The secure-provider implementation and cross-toolchain test project have been removed.
The earlier target evidence remains useful historical proof of the fail-closed experiment,
but it is no longer a pending product milestone.

### Options Debug diagnostics submenu — validated

The September 19, 2026 target pass validated the nested native Debug menu on the real
Server 2003 / Pentium 4 target.

Observed evidence:

- `Options -> Debug` rendered as a native nested submenu,
- the expected actions were visible:
  Runtime Diagnostics, Copy Diagnostic Report, Take Diagnostic Capture,
  Export Browser Diagnostic Report, and Open Diagnostics Folder,
- the diagnostic capture still produced the expected BMP+TXT artifacts,
- the resulting report preserved runtime, backend, Conversation security, and
  file-location state,
- the content-free Conversation probe remained operational,
- Browser Probe remained operational with a real ChatGPT HTTP 200 response.

The Debug submenu is therefore target-green and can evolve as the development feedback
surface for future feature probes.

### LibreWolf browser relay — validated end-to-end on real P4

The first useful text-in/text-out ChatGPT baseline is now validated across the complete
real target path.

The original Selenium/GeckoDriver approach was rejected after real testing because
LibreWolf entered Marionette remote-control mode and ChatGPT would not load the selected
conversation normally. The accepted baseline uses the user's normal LibreWolf process
plus a temporary development WebExtension, a localhost-only session broker, and the
existing P4-facing bridge.

Validated September 19, 2026:

- Visual C++ 7.1 `Debug | Win32` rebuilt the real SalixWeb32 target with **0 errors and
  0 warnings**.
- Native diagnostics reported:
  `Remote Conversation Bridge Backend | SALIX-CONVERSATION/1 ready | browser relay |
  trusted LAN | text only`.
- The active security profile reported:
  `mode content | transport trusted-lan | text yes | attachments no | credentials no |
  session no`.
- The user sent `Hello from Pentium 4` from the native SalixWeb32 Conversation
  composer on Windows Server 2003.
- The modern bridge received that request as request ID 1 with 20 text bytes and no
  attachments, credentials, or session forwarding.
- The localhost broker/WebExtension path operated through normal LibreWolf with no
  Marionette/WebDriver browser mode.
- The authenticated ChatGPT conversation produced the real assistant response.
- The broker returned 469 response bytes, the bridge returned HTTP 200 to the P4, and
  Salix rendered the returned message in the native Conversation view.
- Diagnostic capture remained operational under the configured
  `%APPDATA%\SalixWeb32\Diagnostics` location.

This proves the complete baseline:

```text
native SalixWeb32 / Pentium 4
        -> SALIX-CONVERSATION/1
        -> salix_bridge.py
        -> localhost salix_chat_session.py broker
        -> LibreWolf relay WebExtension
        -> normal authenticated LibreWolf / chatgpt.com
        -> rendered assistant response
        -> semantic Conversation events
        -> native Salix Conversation view
```

Known observations accepted for this baseline:

- response delivery is noticeably slow because the first implementation waits for the
  rendered assistant response to stabilize before returning the completed response,
- true generation-time streaming is not implemented yet,
- text only is enabled; attachments remain deliberately blocked,
- conversation selection is still the currently open ChatGPT thread,
- the original legacy text path exposed mojibake/glyph failures, but the later
  UTF-8 framework / UTF-16 Win32 boundary and glyph-fallback tranche resolved and
  target-validated those issues,
- the relay extension is still loaded as a temporary development extension and must be
  reloaded after LibreWolf restarts.

These are follow-up work items, not failures of the validated architecture.

Regression checklist for the frozen baseline:

1. clean/rebuild `Debug | Win32` with VC7.1 at zero errors/warnings,
2. normal LibreWolf is authenticated with no Marionette/WebDriver banner,
3. relay WebExtension is loaded and ChatGPT tab is open,
4. `salix_chat_session.py` reports the normal-LibreWolf WebExtension path,
5. `salix_bridge.py` reports browser-relay text baseline,
6. P4 Conversation header reaches
   `SALIX-CONVERSATION/1 ready | browser relay | trusted LAN | text only`,
7. a P4 text message appears in the active ChatGPT thread,
8. the real assistant response returns to the native Salix Conversation view,
9. attachments/credentials/session forwarding remain disabled,
10. Browser Probe and the content-free Conversation probe remain available as
    independent diagnostics.


## Relay timing instrumentation — validated on real P4

This tranche must not change the already validated text-only browser-relay behavior. It
adds diagnostic timing metadata only.

### Modern companion validation

1. Check out/pull `dev` on Aurora8.
2. Keep normal LibreWolf + the relay WebExtension running.
3. Restart:
   `python tools\salix_chat_session.py`.
4. Restart:
   `python tools\salix_bridge.py --host 0.0.0.0 --port 8765`.
5. Run:
   `python tools\test_chat_relay.py --message "Salix timing smoke test"`.
6. Require HTTP 200 and the existing semantic events.
7. Require a `relay timing:` block containing at least:
   - `bridge_total_ms`,
   - `broker_total_ms`,
   - `broker_queue_ms`,
   - `browser_total_ms`,
   - `browser_first_response_ms`,
   - `browser_generation_ms`,
   - `browser_stabilization_ms`.
8. Confirm the assistant text still reconstructs exactly as before.

### Real P4 validation

1. Pull `dev` on the P4.
2. Clean/Rebuild `Debug | Win32` in VC7.1 with zero errors/warnings.
3. Send one short real message from native SalixWeb32.
4. Confirm the normal real ChatGPT round-trip still succeeds.
5. Use `Options -> Debug -> Copy Diagnostic Report`.
6. Require:

```text
Conversation Relay Timing
-------------------------
P4 total <N> ms | Relay timing: bridge <N> ms | broker <N> ms | ...
```

7. Record the exact timing line before any optimization work.
8. Confirm the Conversation security profile remains:
   `mode content | transport trusted-lan | text yes | attachments no |
   credentials no | session no`.

Interpretation:

- `browser_first_response_ms` approximates model/service time to first visible assistant
  content after submission,
- `browser_generation_ms` approximates visible response growth after first content,
- `browser_stabilization_ms` measures the deliberate post-change stability wait,
- `broker_queue_ms` measures extension command-poll pickup latency,
- `bridge_total_ms` measures modern-side bridge-visible relay duration,
- `P4 total` measures native user-visible completion and includes P4 event-drain/render
  overhead.

Do not optimize anything in this tranche. First collect a trustworthy baseline.

Observed real-P4 timing baseline on September 19, 2026:

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

The Conversation backend remained
`SALIX-CONVERSATION/1 ready | browser relay | trusted LAN | text only`, and the
security profile remained text-only with attachments/credentials/session disabled.

This run proves the timing metadata survives the complete real target path. The observed
difference between `P4 total` and `bridge` is 38,090 ms. That delta is outside the
modern relay timing window and strongly points at native-side post-response event
draining/presentation as a major latency contributor in the current completed-response
replay design. Treat that as a measured optimization target, not as proof of one specific
renderer function until profiling narrows it further.

The corresponding full VC7.1 `Debug | Win32` rebuild completed with zero errors and
zero warnings. The timing feature is therefore target-green and eligible for promotion
to `main`.

A same-session diagnostic capture also remained healthy: runtime and remote web backend
state were operational, the configured Diagnostics directory was used, and the timing
report remained present in the capture.

## Native Conversation presentation batching — validated on real P4

The measured timing baseline showed approximately 38–47 seconds outside the modern
relay window on the P4. Source inspection found that the completed browser response was
already available in full, but Salix consumed its synthetic `text_delta` events by
rebuilding Markdown/layout presentation after every delta.

The `dev` candidate now preserves every semantic event while coalescing presentation
work to at most one update per native event-drain pass.

Current completed-response behavior should therefore be:

```text
request_started
message_started
text_delta x N
message_completed
        |
        v
append all delta text
        |
        v
one native ConversationView presentation update
```

This does not change the bridge protocol and does not remove incremental semantics.
When true streaming later supplies partial event batches over time, Salix can still
present once per newly arrived batch.

### Real P4 validation

1. Pull the staged `test` candidate on the P4.
2. Clean/Rebuild `Debug | Win32` in VC7.1 and require zero errors/warnings.
3. Keep the already validated Aurora8 relay stack running.
4. Send one ordinary multi-sentence prompt from native SalixWeb32.
5. Confirm the assistant response appears as one completed native update rather than
   visibly crawling into place one tiny fragment at a time.
6. Confirm Unicode, Markdown, selection, scrolling, and security state remain intact.
7. Use `Options -> Debug -> Copy Diagnostic Report`.
8. Require the timing line to include:

```text
native batch <N> deltas -> <M> updates | present <P> ms
```

For the current completed-response relay, the expected result is `M = 1` while
`N` remains greater than one for a normal response.

9. Compare:
   - `P4 total`,
   - `bridge`,
   - `present`,
   - delta count,
   - native presentation update count.
10. Preserve the existing text-only security profile and relay timing fields.

The optimization is successful only if the native response remains semantically and
visually correct while the measured P4-side post-response penalty drops substantially.

### First batching attempt — rejected

The first real-P4 pass rebuilt cleanly under VC7.1 with zero errors and zero warnings,
but the runtime diagnostic reported:

```text
P4 total 71531 ms
bridge 27891 ms
native batch 28 deltas -> 28 updates
present 48 ms
```

This proved that the StatusView batching layer itself was correct but could not coalesce
events because `RemoteConversationBackend::take_event()` intentionally exposed only one
event per runtime update through `event_taken_this_update`.

That throttle stretched an already-complete response across 28 native update cycles.
The measured presentation work itself was only 48 ms, so the large user-visible delay
was dominated by artificial semantic-event pacing rather than renderer CPU cost.

The corrected candidate removes the one-event-per-update gate. The backend now drains
every semantic event already queued by the completed response in one native pass. Future
true streaming remains incremental because only events actually available in a given
backend update can be drained.

### Corrected batching retest — validated

The corrected real-P4 retest rebuilt under Visual C++ 7.1 with zero errors and zero
warnings. The user-visible result was immediate: the completed assistant response appeared
as one native update instead of being drip-fed across synthetic deltas.

The captured diagnostic reported:

```text
P4 total 7406 ms
bridge 7222 ms
broker 7206 ms
queue 415 ms
extension 6785 ms
browser 6767 ms
submit 117 ms
first response 250 ms
generation 4367 ms
stabilize 2033 ms
native batch 27 deltas -> 1 updates
present 0 ms
```

The P4 total now exceeds the bridge total by only 184 ms, compared with the rejected
first pass where the already-complete response was stretched across 28 native update
cycles. The semantic delta count remains greater than one, but all currently available
events are drained and coalesced into one presentation update as intended.

The diagnostic also preserved the validated UTF-8 framework / UTF-16 Win32 presentation
boundary and the existing text-only trusted-LAN Conversation security profile.

This tranche is target-green. Future true streaming remains compatible with the design:
newly arriving event batches can still be presented incrementally, while an already
complete batch is no longer artificially paced across native update cycles.

## UTF-8 / Win32 Unicode boundary — validated on real P4

This tranche keeps framework/wire text in UTF-8 byte strings while making byte boundaries
code-point aware and converting to UTF-16 only at the Win32 presentation/clipboard
boundary.

Implementation scope:

- UTF-8 code points remain atomic during soft wrapping,
- label/input caret navigation does not step into continuation bytes,
- delete/backspace remove whole UTF-8 code points,
- Win32 measurement and drawing use `GetTextExtentPoint32W` / `TextOutW`,
- formatted button text uses `DrawTextW`,
- formatted fonts are created through `CreateFontW`,
- the Win32 clipboard publishes/reads `CF_UNICODETEXT`,
- incoming legacy `CF_TEXT` is converted ACP -> UTF-8,
- native ACP character input is normalized to a Unicode code point then encoded as UTF-8,
- invalid UTF-8 presentation falls back to ACP so older application/path strings are not
  broken while their source APIs remain ANSI.

### Real target checklist

1. Pull the staged `test` candidate on Aurora8 and the P4.
2. Clean/Rebuild `Debug | Win32` in VC7.1 and require zero errors/warnings.
3. Keep the existing browser relay running and send an ASCII-only prompt from the P4
   requesting an exact Unicode response.
4. Use this validation response:

```text
I’m “testing” — café € → ↓
```

5. Require the native Conversation view to display the punctuation/accents/arrows as the
   characters above, with no mojibake sequences such as `Iâ€™m`.
6. Drag-select the Unicode response in SalixWeb32, copy it, paste into Server 2003
   Notepad, and require the same Unicode text.
7. Paste the copied Unicode phrase back into the Salix composer and require it to remain
   intact there.
8. Send that pasted phrase through the relay and confirm the browser thread receives the
   same characters.
9. Use `Options -> Debug -> Copy Diagnostic Report` and require:

```text
Text encoding: UTF-8 framework | UTF-16 Win32 presentation
```

10. Confirm the existing Conversation security profile and relay-timing diagnostics still
    work.
11. Record the new P4 timing line. Unicode correctness must not silently remove the
    timing instrumentation established in the previous tranche.

Font-selection note: the first pass proved that missing-glyph boxes can still occur
even when the Unicode data is correct. A follow-up Server 2003 Notepad check demonstrated
that the target OS can render `→ ↓`, so Salix must provide font fallback rather than
assuming its hard-pinned Tahoma/Courier New face is sufficient. Mojibake remains an
encoding failure; a box that disappears under the Salix fallback chain is a font
selection failure.

Observed real-P4 Unicode validation on September 19, 2026:

- the native Conversation view rendered the smart apostrophe, curly quotes, em dash,
  `é`, and `€` correctly with no mojibake,
- the selected/copied/pasted phrase was sent back through the real browser relay,
- ChatGPT received the exact original UTF-8 text:
  `I’m “testing” — café € → ↓`,
- the diagnostic report declared
  `Text encoding: UTF-8 framework | UTF-16 Win32 presentation`,
- the Conversation security profile remained unchanged,
- relay timing remained present.

The first pass still showed boxes for `→ ↓` inside Salix while Server 2003 Notepad
rendered those same code points correctly. That narrowed the remaining problem to
Salix's hard-pinned font choice rather than the OS or UTF-8 data.

Salix then gained Win32 glyph-aware fallback selection while preserving Tahoma/Courier
New as preferred faces. Because the VC7.1 SDK headers did not declare
`GetGlyphIndicesW`, the glyph probe is resolved dynamically from `gdi32.dll` so the
target does not depend on newer SDK declarations.

The final target retest showed `→ ↓` rendering correctly in:

- the native Conversation `You:` message,
- the native Conversation `Remote:` message,
- and the native composer.

The corresponding VC7.1 `Debug | Win32` rebuild completed with zero errors and zero
warnings. The UTF-8 / Win32 Unicode boundary, Unicode clipboard round-trip, and Salix
glyph-fallback behavior are therefore target-green.

## Persistent file-location regression — Server 2003 R2 validation tranche staged

A September 19, 2026 Server 2003 diagnostic capture exposed a concrete path-ownership
bug. After the native attachment picker had browsed an external RenderWare directory,
SalixWeb32 wrote both diagnostic capture artifacts and the Browser Diagnostic Report
under:

```text
<external RenderWare directory>\diagnostics
```

The old implementation used process current-directory state at export time, so an
unrelated common-dialog navigation could redirect application-owned output.

The corrective tranche now establishes explicit startup roots:

- launch directory captured once for development/local-config discovery,
- executable directory captured independently,
- Standard `user_data_root = %APPDATA%\SalixWeb32`,
- Portable `user_data_root = executable directory` only when launched with `--portable`,
- `settings.ini` beneath the selected data root,
- Diagnostics defaulting to `<user_data_root>\Diagnostics`,
- attachment picker recent-directory history tracked independently,
- first-use attachment browsing defaulting to `%USERPROFILE%` when no history exists,
- `OFN_NOCHANGEDIR` on the native attachment picker,
- diagnostic exporters receiving the configured destination explicitly,
- `Options -> Settings...` displaying application mode, data root, settings path, and
  Diagnostics only; attachment history remains automatic state.

Initial Standard-mode evidence from the real Server 2003 R2 / Pentium 4 target is green:

- VC7.1 `Debug | Win32` rebuilt with zero errors and zero warnings,
- `Options -> Settings...` reported `Application mode: Standard`,
- user data resolved to `C:\Documents and Settings\Administrator\Application Data\SalixWeb32`,
- Diagnostics resolved beneath that root,
- `settings.ini` resolved beneath that root,
- no editable attachment-browser directory was shown in Settings.

A later attachment-relay evidence bundle adds stronger Standard-mode path-isolation
evidence:

- VC7.1 `Debug | Win32` again rebuilt with zero errors and zero warnings,
- the attachment picker recent folder was `X:\NTSHARE\Diagnostics`,
- diagnostic capture/report output still remained under
  `C:\Documents and Settings\Administrator\Application Data\SalixWeb32\Diagnostics`,
- the diagnostic report resolved the separate received-file root as
  `C:\Documents and Settings\Administrator\Application Data\SalixWeb32\Received`.

This proves that attachment browsing no longer redirects application-owned Diagnostics
or Received storage through process current-directory state. Portable-mode validation
and the remaining preference/reset checklist items are still pending.

Target checklist:

1. Clean/Rebuild `Debug | Win32` under VC7.1 with zero errors and zero warnings.
2. Launch normally and open `Options -> Settings...`.
3. Confirm `Application mode: Standard`.
4. Confirm the data root is `%APPDATA%\SalixWeb32`.
5. Confirm the settings file is `%APPDATA%\SalixWeb32\settings.ini`.
6. Confirm Diagnostics defaults to `%APPDATA%\SalixWeb32\Diagnostics`.
7. Confirm the Settings window contains no editable Attachment browser directory.
8. For a first-use test, temporarily remove only the `attachment_directory=` line from
   `settings.ini`, launch SalixWeb32, and open the attachment picker.
9. Confirm that picker starts at `%USERPROFILE%`.
10. Browse to another directory, select a file, reopen the picker, and confirm the new
    directory is remembered automatically.
11. Confirm `settings.ini` now contains the new `attachment_directory=` history value.
12. Change the Diagnostics folder, save/reopen Settings, and confirm the Diagnostics
    choice persists without resetting the remembered attachment directory.
13. Take a diagnostic screenshot and confirm it is written to the configured Diagnostics
    folder, not the attachment directory.
14. Confirm the diagnostic report labels the navigation state as
    `Attachment recent folder`.
15. Export a Browser Diagnostic Report and confirm it uses the same configured Diagnostics
    folder.
16. Confirm `Go to Files` opens the actual configured Diagnostics folder.
17. Use `Restore Defaults`, save, and confirm only Diagnostics returns to
    `%APPDATA%\SalixWeb32\Diagnostics`; attachment history should remain unchanged.
18. Launch `SalixWeb32.exe --portable`.
19. Confirm `Application mode: Portable (--portable)`.
20. Confirm Data root is the directory containing `SalixWeb32.exe`.
21. Confirm the settings file is `<executable_root>\settings.ini`.
22. Confirm Diagnostics defaults to `<executable_root>\Diagnostics`.
23. With no portable `attachment_directory=` history, confirm the first attachment picker
    still starts at `%USERPROFILE%`, then remembers subsequent navigation in the portable
    `settings.ini`.
24. Record the exact Standard and Portable paths from the real Server 2003 R2 target,
    together with the VC7.1 build result and any diagnostic capture/report evidence.

MiniXP is deliberately **not** part of this tranche's acceptance gate. Compatibility
testing there is deferred until the Server 2003 feature set is complete and the MiniXP
environment is repaired/stabilized.

See `docs/FILE_LOCATIONS.md`.


## Bounded bidirectional Conversation file relay — dev candidate pending target validation

The next Conversation tranche extends the validated text-only browser relay with bounded
file attachments while preserving the existing provider-neutral application boundary.

Implementation path:

```text
MessageDraft / ConversationRequest
        -> RemoteConversationBackend
        -> SALIX-CONVERSATION/1
        -> salix_bridge.py
        -> localhost salix_chat_session.py
        -> LibreWolf WebExtension 0.2.7
        -> visible authenticated ChatGPT conversation
        -> returned attachment events
        -> %APPDATA%\SalixWeb32\Received
        -> native ConversationView
```

Current bounds:

```text
maximum files per request: 8
maximum file size:         2 MB
maximum total file bytes:  4 MB
```

The remote security profile must advertise attachments explicitly before Salix accepts
file-bearing content requests. Credentials and browser session state remain disabled.
The content-free probe remains attachment-free.

Native semantics:

- outgoing attachments retain the local `You:` role,
- assistant-returned attachments use the remote `Remote:` role,
- images retain the existing thumbnail / Preview / Open behavior,
- text and generic files retain a filename surrogate plus Open behavior,
- received files are sanitized and stored beneath the active Salix `Received` directory,
- same-name received files must not silently overwrite each other,
- wire/base64 attachment framing remains inside the remote Conversation backend rather
  than leaking into `StatusView` or `ConversationView`.

### Companion validation

1. Pull the staged candidate on the modern companion.
2. Reload the temporary LibreWolf extension and confirm version `0.2.7`.
3. Restart `tools\salix_chat_session.py`.
4. Restart `tools\salix_bridge.py --host 0.0.0.0 --port 8765`.
5. Require bridge health to report:
   `conversation_attachment_forwarding=enabled`.
6. Confirm credentials/session forwarding remain disabled.
7. Create a small text file and run:
   `python tools\test_chat_relay.py --message "Attachment smoke test" --file <path>`.
8. Confirm the visible ChatGPT thread receives that file and the helper completes with
   HTTP 200.

### Real P4 validation

1. Pull the staged `test` candidate.
2. Clean/Rebuild `Debug | Win32` under VC7.1 with zero errors and zero warnings.
3. Confirm the Conversation backend reports `text + files`.
4. Confirm the security profile reports:
   `mode content | transport trusted-lan | text yes | attachments yes | credentials no | session no`.
5. Create a small plain-text file on the P4 and attach it with the existing `+` picker.
6. Send a message with that text file and confirm the file appears in the visible ChatGPT
   conversation rather than only in local Salix presentation.
7. Repeat with a small image and confirm the browser receives the image attachment.
8. Ask the remote side to return a small text file.
9. Confirm Salix renders the returned attachment as `Remote: <filename>`.
10. Confirm the file is written beneath:
    `%APPDATA%\SalixWeb32\Received` in Standard mode.
11. Open the received text file through the attachment context action and confirm the
    platform default application opens it.
12. Return a small image and confirm thumbnail / Preview / Open still work.
13. Repeat a same-name returned file and confirm Salix creates a unique destination rather
    than overwriting the existing received file.
14. Confirm diagnostics include a file count in the native timing line.
15. Confirm Browser Probe and the content-free Conversation probe still work.
16. Confirm oversized/over-count requests fail cleanly without partial forwarding.

### First modern attachment smoke observation

The first modern attachment smoke pass proved that file injection itself worked: the
test file appeared visibly in the ChatGPT composer and reached the conversation after a
manual click on Send. The extension did not complete the final submit action
programmatically.

The follow-up extension `0.2.1` hardens that boundary. It now:

- re-finds the live Send control after upload readiness,
- accepts current send-button/test-id/ARIA/title/submit-button shapes,
- excludes Stop controls,
- retries a bounded click sequence if the page ignores the first click,
- verifies submission by observing a new user turn or cleared/changed composer content,
- reports an explicit relay error instead of silently assuming submission succeeded.

The follow-up modern retest is now green. With WebExtension `0.2.1`, the smoke helper:

```bat
python tools\test_chat_relay.py --message "Please confirm you received this file." --file attachment_test.txt
```

visibly attached `attachment_test.txt`, submitted the message automatically without any
manual Send click, and the file arrived successfully in the ChatGPT conversation with
the expected contents. The outgoing modern browser attachment path is therefore green.

The native P4 outgoing attachment gate is now green. A text file sent from the real
Pentium 4 through SalixWeb32 arrived successfully in the ChatGPT conversation with the
expected file contents. This validates the native path:

```text
P4 MessageDraft / ConversationRequest
    -> RemoteConversationBackend
    -> SALIX-CONVERSATION/1
    -> bridge / localhost broker
    -> LibreWolf extension 0.2.1
    -> ChatGPT attachment
```

The remaining acceptance gate is remote-to-P4 returned file storage/presentation under
the application-owned `Received` directory.

### First reverse-file observation and correction

The first returned-file attempt did **not** create a native Remote attachment. The P4
evidence bundle confirmed that the configured Received root existed and was reported
correctly, but `SalixWeb32_return_test.txt` was absent from the evidence and no
`Remote:` file entry appeared in the native conversation.

The browser-side cause was explicit in the extension: `looksLikeAttachmentAnchor()`
recognized ChatGPT `sandbox:` file links as attachment candidates, but
`attachmentCandidateUrls()` immediately discarded every `sandbox:` URL before
download collection. The reverse semantic attachment event therefore never existed; the
P4 storage code was not the failing boundary.

WebExtension `0.2.2` corrects that boundary without forwarding browser credentials.
For a returned `sandbox:` attachment it now:

1. arms a bounded WebExtension download capture,
2. lets normal LibreWolf activate the ChatGPT attachment link,
3. waits for the browser-owned authenticated download to complete,
4. returns the completed local download path to the localhost broker,
5. lets the broker read/bound/base64-package the file,
6. forwards the existing semantic attachment response toward the P4,
7. removes the temporary modern-side browser download after the broker accepts it.

The next target retest must confirm `Remote: SalixWeb32_return_test.txt` plus the actual
file under the Standard-mode `%APPDATA%\SalixWeb32\Received` directory.

### Second reverse-file observation — 0.2.2

The `0.2.2` target retest again returned the assistant text but no file. Native
diagnostics reported:

```text
Conversation backend: ... text + files
Conversation security: ... attachments yes ...
native batch 28 deltas -> 1 updates
present 0 ms
files 0
Received files folder:
C:\Documents and Settings\Administrator\Application Data\SalixWeb32\Received
```

The modern broker/extension request also completed normally rather than waiting for the
configured 30-second download-capture timeout. This means the browser download path was
not entered: attachment discovery produced no candidate element.

The likely rendered-DOM boundary is the file card being outside the narrower
`[data-message-author-role='assistant']` node used by `assistantNodes()`. Version
`0.2.3` changes returned-file discovery to:

- scan the whole containing assistant conversation turn,
- inspect anchors, buttons, role-buttons, and data-link controls,
- recognize download labels/titles and file-like labels/URLs,
- poll briefly for a late-rendered attachment control when the response text mentions a
  filename,
- report scan/candidate/sandbox/download/fetch counts and bounded error details to the
  localhost broker.

The next retest should capture the `[chat-session] attachment capture ...` telemetry
line even if file relay still fails, so another failure can be localized without guessing.

### Third reverse-file observation — 0.2.3

The `0.2.3` retest returned assistant text but still reported `files 0` on the P4.
Unlike `0.2.2`, the modern browser visibly opened the returned file and then displayed
LibreWolf's native Save As dialog. The user closed that dialog rather than manually saving
the relay file, which is correct: manual Save As is not part of the relay contract.

Native timing showed approximately 52.8 s of extension work versus 22.7 s of browser
generation/presentation, consistent with the bounded download-capture waits being blocked
behind the interactive Save As flow.

The same returned text also exposed a Markdown presentation bug: Windows paths such as
`C:\Documents and Settings\...\Received` lost their backslashes. The inline
Markdown formatter was treating every backslash as an escape character. The corrected
formatter now consumes a backslash only when the following character is Markdown
punctuation; literal Windows path separators before ordinary letters are preserved.

WebExtension `0.2.4` changes returned-file capture to:

- prioritize explicit Download controls over generic file cards,
- treat a generic file card as a preview opener when necessary,
- locate the explicit Download control after preview opens,
- cancel/erase the interactive download generated by the page,
- restart the captured authenticated URL with `browser.downloads.download(...,
  saveAs:false)`,
- relay the managed temporary file through the existing broker and semantic attachment
  event path,
- keep capture telemetry for the next target retest.

Because the Markdown correction changes native C++, the `0.2.4` candidate requires a
fresh VC7.1 P4 rebuild before the next full retest.

### Fourth reverse-file observation — 0.2.4

The `0.2.4` retest confirmed that the Markdown backslash correction is green on the real
P4: Windows paths such as
`C:\Documents and Settings\Administrator\Application Data\SalixWeb32\Received`
now retain every literal backslash in both local and remote conversation text.

Returned-file capture still failed. LibreWolf again displayed its native Save As dialog
and the P4 completed with `files 0`. Diagnostics still reported the correct
`text + files` backend/security state and the correct application-owned Received
directory.

Version `0.2.5` changes the browser boundary again:

- the extension may open the returned file preview only to expose the real Download
  control,
- it does **not** click that Download control,
- it extracts an HTTP(S) URL from the control/anchor/data URL attributes,
- the background extension calls `downloads.download({ url, saveAs:false })` without a
  `filename` option,
- if no HTTP(S) URL is exposed, the attempt fails with telemetry instead of creating a
  Save As dialog,
- the extension attempts to close the preview after capture.

No native C++ changes are part of `0.2.5`; once Aurora8 is updated/reloaded, the P4 may
retest using the already rebuilt Markdown-fixed binary.

### Fifth reverse-file observation — 0.2.5

The `0.2.5` retest achieved one important correction: **no Save As dialog appeared**.
The browser still opened the returned-file preview, which is currently accepted as useful
observable behavior during relay development.

The reverse transfer itself remained red. Native diagnostics reported:

```text
Conversation backend: ... text + files
Conversation security: ... attachments yes ...
native batch 28 deltas -> 1 updates
present 0 ms
files 0
Received files folder:
C:\Documents and Settings\Administrator\Application Data\SalixWeb32\Received
```

No `Received` directory existed on disk after the request. This is expected and should
not be manually corrected: `StatusView::save_received_attachment()` calls
`ensure_received_directory()` only when an `event_attachment` actually arrives, and
that helper creates the directory with `CreateDirectoryA`.

The modern `0.2.5` telemetry localized the remaining browser boundary:

```text
candidates_seen=2
preview_open_attempts=1
preview_download_controls=1
download_url_candidates=0
download_capture_attempts=0
download_capture_successes=0
attachments_collected=0
```

The explicit ChatGPT Download control therefore exists but exposes no HTTP(S) URL in the
DOM. Version `0.2.6` adds a temporary blocking Firefox `webRequest` listener during
returned-file capture. It captures/cancels only the actual ChatGPT/oaiusercontent
file-content request generated by the Download action, then replays that signed URL
through the existing managed `saveAs:false` download path. New telemetry fields include
`intercept_capture_attempts`, `intercept_capture_successes`, and
`intercepted_requests`.

No native C++ changes are part of `0.2.6`; the already rebuilt P4 candidate can be reused.

### Sixth reverse-file observation — 0.2.6

The `0.2.6` target retest is the first successful reverse **byte transport**.

Modern telemetry reported successful request interception and managed downloads. The
bridge returned two attachments. Native diagnostics on the real P4 reported:

```text
Conversation backend: ... text + files
Conversation security: ... attachments yes ...
native batch 27 deltas -> 1 updates
present 0 ms
files 2
Received files folder:
C:\Documents and Settings\Administrator\Application Data\SalixWeb32\Received
```

The P4 automatically created the `Received` directory as designed. It contained
`content` and `content(1)`. Both files were 425 bytes and byte-for-byte identical to
the expected returned test payload, proving the complete browser -> broker ->
SALIX-CONVERSATION/1 -> native filesystem byte path.

The remaining `0.2.6` failures are metadata-level:

- one semantic file was captured twice because two browser controls represented it,
- the signed download endpoint supplied the generic leaf name `content`,
- losing the original `.txt` filename also prevented useful file association/Open
  behavior.

Version `0.2.7` corrects that layer by preserving the assistant file-card filename
through the preview/download capture, falling back to unique filename mentions from the
assistant response, deduplicating candidate controls by semantic name, and applying an
additional broker duplicate guard for identical same-named payloads. MIME inference now
prefers the semantic filename over the temporary browser download name.

No new native C++ changes are part of `0.2.7`; reuse the existing P4 binary.

### Seventh reverse-file observation — 0.2.7

The `0.2.7` target retest is green for the complete basic bidirectional text-file path.

Modern telemetry reported:

```text
attachments_collected=1
candidates_seen=2
duplicate_candidates_skipped=1
duplicate_attachments_skipped=0
intercept_capture_attempts=1
intercept_capture_successes=1
intercepted_requests=1
managed_download_successes=1
```

The bridge returned exactly one semantic attachment. Native diagnostics on the real P4
reported:

```text
Conversation backend: ... text + files
Conversation security: ... attachments yes ...
native batch 28 deltas -> 1 updates
present 0 ms
files 1
Received files folder:
C:\Documents and Settings\Administrator\Application Data\SalixWeb32\Received
```

The P4 stored exactly one file named
`SalixWeb32_reverse_filename_0_2_7.txt` under the application-owned `Received`
directory. Windows identified it as a Text Document and the native/default Open path
launched Notepad successfully. The file contents matched the expected returned payload.

This closes the basic bidirectional text-file tranche:

- native P4 -> ChatGPT file send: green,
- automatic browser submission: green,
- ChatGPT -> browser returned-file capture: green,
- Save As suppression: green,
- one semantic attachment per returned file: green,
- semantic filename/extension preservation: green,
- native `Received` directory creation/storage: green,
- default-application Open for the returned text file: green.

A follow-up curiosity test attempted to send the entire evidence bundle back through the
P4 client in one message. The client correctly rejected the request because the bounded
relay contract allows at most 8 attachments. This is expected contract enforcement, not
a regression in the validated file path. Evidence bundles larger than 8 files must be
split across messages unless the bounded policy is deliberately revised in a future
tranche.

### Single-PNG outgoing submission observation — 0.2.8 pending

The first bounded PNG round-trip request did **not** reach reverse-image validation.
Companion logs identify the failed request as:

```text
[chat-session] request id=1 text_bytes=87 attachments=1
[chat-session] request id=1 failed: RuntimeError: Error: ChatGPT Send control did not accept the relay submission.
```

The bridge translated that broker failure into HTTP 503 for the P4 request. In the same
log set, a five-attachment request containing images succeeded with approximately
`browser_submit_ms=7650`, while the extension's unnamed-attachment fallback had allowed
submission readiness after only 2500 ms.

The `0.2.8` candidate therefore:
- keeps named-file readiness behavior unchanged,
- requires filename-less/image-thumbnail uploads to reach at least an 8-second settle
  point,
- additionally requires the Send control to remain usable for 1.5 seconds,
- makes no native C++ changes,
- leaves the validated `0.2.7` reverse-file capture logic unchanged.

Acceptance for the retest:
- no HTTP 503 from outgoing PNG submission,
- request appears in ChatGPT with the PNG attached,
- assistant can return the bounded PNG as a normal file,
- only then evaluate P4 `files 1`, preserved `.png` filename, Received storage,
  inline thumbnail, Preview, Open, and aspect ratio.

### Single-PNG outgoing submission observation — 0.2.8 rejected / 0.2.9 pending

The fresh-client `0.2.8` retest still failed after the P4 successfully staged and
displayed the local PNG. Companion logs again showed:

```text
[chat-session] request id=1 text_bytes=88 attachments=1
[chat-session] request id=1 failed: RuntimeError: Error: ChatGPT Send control did not accept the relay submission.
```

The bridge surfaced the corresponding conversation request as HTTP 503. The independent
Browser Probe initialized cleanly and returned HTTP 200, so fresh Salix startup and the
unauthenticated probe path were not the cause.

Code review found the `0.2.8` defect: `injectAttachments()` returned immediately when
all attachment filenames were visible, before consulting the new filename-less
8-second settle path. A single PNG whose filename appeared quickly could therefore retain
the original race.

The `0.2.9` candidate:
- determines image settling from MIME type rather than filename visibility,
- forces all `image/*` uploads through the 8-second / 1.5-second stable-Send gate,
- retains the fast path for validated named non-image files,
- observes attachment submit acceptance for up to 5 seconds,
- treats active generation as positive submission evidence,
- attempts one `form.requestSubmit()` fallback after an unaccepted normal click,
- includes a compact `submit_state={...}` diagnostic in any final submission failure.

No native P4 rebuild is required. The same bounded PNG and request text should be reused.

### Single-PNG round-trip observation — 0.2.9 transport green

The `0.2.9` target retest cleared the browser submission race and completed a full
ordinary-PNG transport cycle.

Aurora companion telemetry reported:

```text
request id=1 text_bytes=88 attachments=1
attachment capture attachments_collected=1
candidates_seen=2
duplicate_candidates_skipped=1
intercept_capture_attempts=1
intercept_capture_successes=1
intercepted_requests=1
managed_download_successes=1
request id=1 response_bytes=587 attachments=1
POST /v1/message ... 200
```

The bridge reported:

```text
browser relay request id=1 text_bytes=88 attachments=1
browser relay response id=1 text_bytes=587 attachments=1
POST /v1/conversation/message ... 200
```

The real P4 then rendered the returned remote PNG inline and stored exactly one
`SalixWeb32_remote_image_return_test.png` under the application-owned `Received`
directory. The returned PNG captured in the evidence bundle was 263,102 bytes and
byte-for-byte identical to the file sent by the assistant (matching SHA-256), proving
binary round-trip integrity as well as semantic filename preservation.

Validated in this pass:

- P4 single-PNG automatic browser submission: green,
- no HTTP 503: green,
- assistant ordinary PNG capture: green,
- duplicate browser candidate suppression: green,
- reverse PNG semantic event transport: green,
- native Received storage: green,
- preserved `.png` filename: green,
- byte-for-byte returned PNG integrity: green,
- inline remote thumbnail rendering: green,
- visible aspect ratio appears correct in the conversation surface.

Final presentation validation is now green. A real-P4 video capture confirms:

- right-click / attachment action opens the returned PNG in Salix's native Preview,
- the native Preview renders the image correctly,
- the external/default-application Open action launches the Windows image viewer,
- the same returned PNG remains visually proportionate in both presentation paths.

This completes the ordinary PNG bidirectional tranche on the real Pentium 4 / Windows
Server 2003 target.

The generated-image-card browser shape remains a separate future provider-adapter case;
this result validates an ordinary returned PNG file attachment only.

MiniXP is not part of this tranche's acceptance gate.
