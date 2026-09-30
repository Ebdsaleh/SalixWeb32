#!/usr/bin/env python3
"""Unified modern-side server for SalixWeb32 conversation relay.

One process owns both network boundaries:

- the trusted-LAN Salix bridge on --host/--port (default 127.0.0.1:8765),
- the localhost-only LibreWolf WebExtension endpoint on 127.0.0.1:8766.

Both listeners share one RelayState. The LAN bridge calls that state directly
in-process for conversation work; it does not make a second localhost HTTP hop.
The localhost HTTP listener remains only because the normal LibreWolf
WebExtension needs a browser-accessible endpoint.

The historical salix_bridge.py and salix_chat_session.py entry points remain
available as diagnostic/fallback tools.
"""

from __future__ import annotations

import argparse
import threading

from http.server import ThreadingHTTPServer

import salix_bridge
import salix_chat_session


EXTENSION_HOST = "127.0.0.1"
EXTENSION_PORT = salix_chat_session.DEFAULT_PORT


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run the unified SalixWeb32 modern-side chat server."
    )
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="LAN bridge listen address (use 0.0.0.0 only on a trusted LAN)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8765,
        help="LAN bridge listen port (default: 8765)",
    )
    args = parser.parse_args()

    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535")

    state = salix_chat_session.RelayState(
        salix_chat_session.DEFAULT_RESPONSE_TIMEOUT_SECONDS
    )

    extension_server = salix_chat_session.ChatSessionServer(
        (EXTENSION_HOST, EXTENSION_PORT),
        salix_chat_session.ChatSessionHandler,
        state,
    )
    extension_server.daemon_threads = True

    try:
        bridge_server = ThreadingHTTPServer(
            (args.host, args.port),
            salix_bridge.SalixBridgeHandler,
        )
    except Exception:
        extension_server.server_close()
        raise

    bridge_server.daemon_threads = True

    # Install the same broker state into the LAN bridge. Conversation requests
    # now call RelayState.submit_message() directly inside this process.
    salix_bridge.CHAT_WORKER_STATE = state

    extension_thread = threading.Thread(
        target=extension_server.serve_forever,
        name="salix-extension-endpoint",
        daemon=True,
    )
    extension_thread.start()

    print("SalixWeb32 Chat Server")
    print("======================")
    print(f"Bridge protocol       : {salix_bridge.PROTOCOL}")
    print(f"Conversation protocol : {salix_bridge.CONVERSATION_PROTOCOL}")
    print(
        "LAN bridge endpoint    : "
        f"http://{args.host}:{args.port}"
    )
    print(
        "Extension endpoint     : "
        f"http://{EXTENSION_HOST}:{EXTENSION_PORT} (localhost only)"
    )
    print("Relay topology         : one process | shared RelayState")
    print("Bridge -> broker hop   : direct in-process call")
    print("Browser control        : normal LibreWolf WebExtension")
    print("Authentication         : existing LibreWolf profile/session")
    print("Forwarding             : text + bounded files + assistant files")
    print("Credentials/session    : never exposed to the LAN bridge")
    print()
    print("LibreWolf must already be running normally.")
    print("Load tools\\librewolf_chat_relay_extension as a temporary add-on.")
    print("When the extension heartbeat sees the ChatGPT composer, relay is ready.")
    print("Press Ctrl+C to stop the server.")

    try:
        bridge_server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping SalixWeb32 chat server.")
    finally:
        bridge_server.shutdown()
        bridge_server.server_close()
        extension_server.shutdown()
        extension_server.server_close()
        extension_thread.join(timeout=2.0)
        salix_bridge.CHAT_WORKER_STATE = None

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
