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

Secondary smoke target:

```text
MiniXP on the same Pentium 4 hardware
```

MiniXP results are smoke tests and do not replace testing on a clean retail Windows XP installation.

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
- some non-ASCII punctuation/symbols can display as mojibake in the legacy native text
  path and require a later encoding/presentation pass,
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


## Persistent file-location regression — pending target validation

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
24. Repeat the smoke pass on MiniXP after Server 2003 is green.

See `docs/FILE_LOCATIONS.md`.
