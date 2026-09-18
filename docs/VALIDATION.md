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

The resulting target build was reported **more responsive**, and Raw copy correctness is
now confirmed. It is **not yet considered fully natural-feeling** on the Pentium 4.
Generic Win32 formatted-text rendering remains an active performance investigation.

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
- the latest large-text performance work is improved but still requires further target
  profiling,
- MiniXP coverage is not implied by Server 2003 validation unless explicitly recorded.

For subsequent tranches, the real Pentium 4 remains authoritative.
