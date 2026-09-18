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

Enabled:

- current open ChatGPT thread,
- Salix user text -> normal LibreWolf ChatGPT composer,
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

If the extension is missing, the localhost worker remains healthy but reports
`extension_not_connected`.

If no ChatGPT composer is available, the worker reports
`chatgpt_composer_not_ready`.

If page interaction fails after a request begins, the extension returns a failure to the
localhost broker, which propagates through `salix_bridge.py` into the existing
Conversation failure event path.

The ordinary LibreWolf window remains visible throughout, so browser-side failures can be
inspected directly.

## Target validation

See `docs/VALIDATION.md` for the current P4 checklist.
