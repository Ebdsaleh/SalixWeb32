# SalixWeb32 LibreWolf Chat Relay Extension

This development extension connects the user's **normal LibreWolf process** to the
localhost-only `salix_chat_session.py` broker.

It deliberately does not use Selenium, GeckoDriver, Marionette, or a remote-control
browser process.

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
   python tools\salix_chat_session.py
   ```

The temporary extension remains installed until LibreWolf is restarted. During
development, reload it from `about:debugging` after editing its files.

## Scope

The extension requests access only to:

- `chatgpt.com` / `www.chatgpt.com`, and
- the localhost Salix broker at `127.0.0.1:8766`.

The extension does not read or transmit account credentials, browser cookies, or browser
storage. It receives message text from the local broker, enters it into the current
ChatGPT composer, observes the rendered assistant response, and returns that response
text to the local broker.

## Current limitations

- one active ChatGPT browser conversation at a time,
- text only,
- temporary developer installation,
- completed rendered response returned after generation stabilizes,
- no attachment transfer yet,
- no ChatGPT thread picker in Salix yet.
