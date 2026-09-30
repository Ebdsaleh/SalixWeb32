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
import atexit
from datetime import datetime, timezone
import os
from pathlib import Path
import sys
import threading

from http.server import ThreadingHTTPServer

import salix_bridge
import salix_chat_session


EXTENSION_HOST = "127.0.0.1"
EXTENSION_PORT = salix_chat_session.DEFAULT_PORT


_session_log_file = None
_session_log_path = None
_original_stdout = None
_original_stderr = None


class _TeeStream:
    def __init__(self, console, log_file, lock) -> None:
        self.console = console
        self.log_file = log_file
        self.lock = lock

    @property
    def encoding(self):
        return getattr(self.console, "encoding", None)

    def write(self, text: str) -> int:
        if not text:
            return 0

        with self.lock:
            self.console.write(text)
            self.log_file.write(text)

            if "\n" in text:
                self.log_file.flush()

        return len(text)

    def flush(self) -> None:
        with self.lock:
            self.console.flush()
            self.log_file.flush()

    def isatty(self) -> bool:
        isatty = getattr(self.console, "isatty", None)
        return bool(isatty()) if isatty is not None else False

    def fileno(self) -> int:
        return self.console.fileno()


def _close_session_log() -> None:
    global _session_log_file
    global _original_stdout
    global _original_stderr

    if _session_log_file is None:
        return

    try:
        timestamp = datetime.now(timezone.utc).isoformat(
            timespec="milliseconds"
        ).replace("+00:00", "Z")
        print(
            "SalixWeb32 chat_server.py session end utc="
            + timestamp
        )
        print("=" * 72)
        sys.stdout.flush()
        sys.stderr.flush()
    finally:
        sys.stdout = _original_stdout
        sys.stderr = _original_stderr
        _session_log_file.close()
        _session_log_file = None


def _install_session_log() -> Path:
    global _session_log_file
    global _session_log_path
    global _original_stdout
    global _original_stderr

    user_profile = os.environ.get("USERPROFILE")
    desktop = (
        Path(user_profile) / "Desktop"
        if user_profile
        else Path.home() / "Desktop"
    )
    desktop.mkdir(parents=True, exist_ok=True)
    log_path = desktop / "session.log"

    log_file = log_path.open(
        "a",
        encoding="utf-8",
        errors="replace",
        buffering=1,
    )
    lock = threading.RLock()

    _session_log_file = log_file
    _session_log_path = log_path
    _original_stdout = sys.stdout
    _original_stderr = sys.stderr

    sys.stdout = _TeeStream(_original_stdout, log_file, lock)
    sys.stderr = _TeeStream(_original_stderr, log_file, lock)
    atexit.register(_close_session_log)

    timestamp = datetime.now(timezone.utc).isoformat(
        timespec="milliseconds"
    ).replace("+00:00", "Z")

    print()
    print("=" * 72)
    print(
        "SalixWeb32 chat_server.py session start utc="
        + timestamp
    )
    print("Session log           : " + str(log_path))
    print("=" * 72)

    return log_path

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
    session_log_path = _install_session_log()


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
    print("Session log            : " + str(session_log_path))
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
