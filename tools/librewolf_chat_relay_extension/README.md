# SalixWeb32 LibreWolf Chat Relay Extension

This development extension connects the user's **normal LibreWolf process** to the
localhost-only Salix relay endpoint.

The normal modern-side entry point is now:

```bat
python tools\chat_server.py --host 0.0.0.0 --port 8765
```

`chat_server.py` owns both the trusted-LAN P4 listener and the localhost-only extension
listener through one shared `RelayState`. The historical
`salix_chat_session.py` and `salix_bridge.py` entry points remain diagnostic/fallback
tools.

The extension deliberately does not use Selenium, GeckoDriver, Marionette, or a
remote-control browser process.

## Load for development

1. Start LibreWolf normally with the profile you already use for ChatGPT.
2. Open:

   ```text
   about:debugging#/runtime/this-firefox
   ```

3. Click **Load Temporary Add-on...**
4. Select:

   ```text
   tools\librewolf_chat_relay_extension\manifest.json
   ```

5. Keep LibreWolf running.
6. Open the ChatGPT conversation you want Salix to use.
7. Start:

   ```bat
   python tools\chat_server.py --host 0.0.0.0 --port 8765
   ```

The temporary extension remains installed until LibreWolf is restarted. During
development, reload it from `about:debugging` after editing extension files.

## Scope

The extension requests access only to:

- `chatgpt.com` / `www.chatgpt.com`, and
- the localhost Salix endpoint at `127.0.0.1:8766`.

The extension does not export account credentials, browser cookies, or browser storage
to Salix. It receives semantic message text plus bounded attachment payloads from the
local relay endpoint, enters them into the visible ChatGPT composer, observes the
rendered assistant response, captures supported returned files, and returns semantic
text/files to the local relay.

Current bounded attachment policy:

```text
maximum files       8
maximum per file    2 MB
maximum aggregate   4 MB
```

## Current main baseline

WebExtension `0.4.1` is the current `main` browser-relay baseline and adds attachment-aware rendered-response anchoring.

For an attachment-bearing request, the rendered user turn may contain attachment-card
text and therefore no longer exactly equal the original submitted text. 0.4.1 counts
rendered user-role turns before submission and anchors the response region to the newly
appended user-role node after submission. The whole-thread response-diff fallback remains
disabled for attachment-bearing requests because attachment DOM mutation can contain
stale conversation text/provider chrome.

Real Pentium 4 validation is green for:

- ordinary completed text response return,
- one attachment-bearing returned-response anchor,
- one returned PNG captured through the managed-download path,
- native `files 1` delivery,
- byte-identical PNG round trip,
- one 2-attachment outbound request,
- one superseding follow-up request from the 0.4.0 line.

## Current limitations

- one active ChatGPT browser conversation/thread at a time,
- temporary developer installation,
- completed rendered response is returned after stabilization rather than true
  incremental provider streaming,
- unlimited/concurrent follow-up depth is not validated,
- outbound multi-attachment Send acceptance remains an active edge case: one real-P4
  4-attachment request failed with `send=no`, while a following 2-attachment request
  succeeded,
- ChatGPT may show a false-positive download-failure toast when the relay intercepts a
  returned-file request even though managed capture succeeds,
- no ChatGPT thread picker in Salix yet.

## Diagnostics

The unified server mirrors its console stream to:

```text
%USERPROFILE%\Desktop\session.log
```

in append mode. Share this file when a relay failure needs detailed timing/state
inspection.
