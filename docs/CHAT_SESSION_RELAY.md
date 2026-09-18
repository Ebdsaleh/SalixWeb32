# LibreWolf Chat Session Relay

This document describes the first real ChatGPT text-in/text-out baseline for SalixWeb32.

## Goal

The user should be able to type in the native SalixWeb32 Conversation interface on the
Pentium 4 and receive the real ChatGPT response back in the native Salix conversation
view.

The modern machine is temporary development scaffolding. It owns the current web browser
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
        |
        | Selenium / GeckoDriver
        v
    visible LibreWolf
        |
        v
    chatgpt.com
```

`salix_bridge.py` remains the only P4-facing listener.

`salix_chat_session.py` is intentionally localhost-only. It owns the visible browser
instance and a dedicated persistent LibreWolf profile.

## Authentication

Authentication is manual.

The user logs into ChatGPT directly inside the visible LibreWolf window. SalixWeb32 and
`salix_bridge.py` do not receive or store:

- account passwords,
- MFA values,
- browser cookies,
- authorization headers,
- local/session storage.

The worker API exposes only readiness plus message/response text.

## First baseline scope

Enabled:

- current open ChatGPT thread,
- Salix user text -> browser composer,
- rendered assistant text -> Salix semantic events,
- repeated text requests in the same browser session.

Not enabled yet:

- thread picker/new-thread control from Salix,
- attachments,
- file downloads,
- credentials/session transfer,
- generation-time byte streaming.

The first pass waits for the rendered assistant response to stabilize. The bridge then
divides the completed response into bounded `text_delta` events so the existing native
incremental presentation path is exercised.

## Modern-machine setup

Install/update Selenium once:

```bat
tools\setup_chat_session.bat
```

Selenium can drive Firefox-family browsers using GeckoDriver and a custom browser binary.
The worker checks common LibreWolf installation locations. An explicit binary path can
still be used:

```bat
python tools\salix_chat_session.py --browser "C:\Program Files\LibreWolf\librewolf.exe"
```

By default the worker now discovers and reuses the **installed LibreWolf per-install
default profile**, matching the profile-selection model used by current Firefox-family
browsers. It first checks the `[Install...]` default recorded in LibreWolf's
`profiles.ini` / `installs.ini`, then falls back to the older profile-level
`Default=1` marker only when no per-install default exists. This is intended to select
the same normal profile LibreWolf itself opens, including the user's authenticated
ChatGPT browser session.

The worker prints:

```text
Profile source : ...
Profile path   : ...
```

before launching so the selected browser identity is explicit.

An exact profile can still be selected when needed:

```bat
python tools\salix_chat_session.py --profile "C:\Users\...\AppData\Local\librewolf\Profiles\<profile>"
```

Do not open the selected profile in two LibreWolf processes at once. Close ordinary
LibreWolf, then start the worker:

```bat
python tools\salix_chat_session.py
```

The visible automated LibreWolf should reuse the existing ChatGPT login. If no installed
profile can be discovered, the older `--prepare-login` mode remains available as a
fallback rather than silently requiring another account login.

Then start the existing bridge in another terminal:

```bat
python tools\salix_bridge.py --host 0.0.0.0 --port 8765
```

Keep the existing firewall rule restricted to the P4. Port 8766 should not be exposed to
the LAN.

Before involving the P4, the complete modern-side chain can be checked locally:

```bat
python tools\test_chat_relay.py
```

That prints bridge health. Once `conversation_browser_session=ready` appears, an
optional real-message smoke test is:

```bat
python tools\test_chat_relay.py --message "Hello from the Salix relay smoke test"
```

The message should appear in the visible ChatGPT conversation and the resulting assistant
text should be printed in the terminal through the same semantic framing used by the P4.

## Relay health contract

When the worker is reachable and the ChatGPT composer is visible, bridge health includes:

```text
conversation_relay=enabled
conversation_protocol=SALIX-CONVERSATION/1
conversation_mode=browser_relay
conversation_text_forwarding=enabled
conversation_attachment_forwarding=disabled
conversation_credential_forwarding=disabled
conversation_session_forwarding=disabled
conversation_transport_security=trusted_lan
conversation_browser_session=ready
```

The P4 backend requires this exact policy before sending text.

## Browser interaction strategy

The worker prefers stable semantic/browser attributes rather than screen coordinates.

Composer lookup includes the current ChatGPT textarea/name/test-id forms and a
contenteditable fallback. Submission prefers the page's enabled Send control and falls
back to Enter.

Assistant extraction prefers `data-message-author-role="assistant"` and then a
conversation-turn/Markdown fallback. Buttons and accessibility-only decorative elements
are removed from the copied response text.

These selectors are intentionally isolated in the worker so ordinary website markup
changes do not require changes to the VC7.1 application.

## Failure behavior

If login is incomplete or no conversation composer is visible, health reports the
browser session as not ready and Salix does not dispatch the message.

If browser interaction fails after a request begins, the bridge reports the failure to
the existing Conversation event/error path. The visible browser remains open so the
failure can be inspected directly.

## Target validation

See `docs/VALIDATION.md` for the current P4 checklist.
