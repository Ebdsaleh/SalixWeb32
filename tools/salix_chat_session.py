#!/usr/bin/env python3
"""Local broker between the SalixWeb32 bridge and a normal LibreWolf extension.

This process does NOT launch or automate LibreWolf.

The user's ordinary LibreWolf process owns the authenticated ChatGPT session.
A small WebExtension loaded into that browser communicates with this localhost-only
broker. The P4-facing salix_bridge.py remains a separate process.
"""

from __future__ import annotations

import argparse
import json
import sys
import threading
import time
from dataclasses import dataclass
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from typing import Any

SESSION_PROTOCOL = "SALIX-CHAT-SESSION/1"
EXTENSION_PROTOCOL = "SALIX-CHAT-EXTENSION/1"
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8766
DEFAULT_RESPONSE_TIMEOUT_SECONDS = 180.0
MAX_MESSAGE_BYTES = 128 * 1024
HEARTBEAT_STALE_SECONDS = 4.0


def _json_bytes(value: dict[str, Any]) -> bytes:
    return json.dumps(value, ensure_ascii=False).encode("utf-8")


@dataclass
class PendingRequest:
    request_id: int
    text: str
    delivered: bool = False
    completed: bool = False
    response_text: str = ""
    error_text: str = ""


class RelayState:
    def __init__(self, response_timeout_seconds: float) -> None:
        self.response_timeout_seconds = response_timeout_seconds
        self.condition = threading.Condition()
        self.pending: PendingRequest | None = None

        self.last_heartbeat = 0.0
        self.extension_version = ""
        self.composer_ready = False
        self.current_url = ""
        self.title = ""
        self.last_error = ""

    def update_heartbeat(
        self,
        extension_version: str,
        composer_ready: bool,
        current_url: str,
        title: str,
        error_text: str,
    ) -> None:
        with self.condition:
            self.last_heartbeat = time.monotonic()
            self.extension_version = extension_version
            self.composer_ready = composer_ready
            self.current_url = current_url
            self.title = title
            self.last_error = error_text
            self.condition.notify_all()

    def get_status(self) -> dict[str, Any]:
        with self.condition:
            age = (
                time.monotonic() - self.last_heartbeat
                if self.last_heartbeat > 0
                else None
            )

            extension_connected = (
                age is not None and
                age <= HEARTBEAT_STALE_SECONDS
            )

            if not extension_connected:
                session_status = "extension_not_connected"
                session_ready = False
            elif not self.composer_ready:
                session_status = "chatgpt_composer_not_ready"
                session_ready = False
            else:
                session_status = "ready"
                session_ready = True

            return {
                "protocol": SESSION_PROTOCOL,
                "status": "ok",
                "browser": "LibreWolf",
                "transport": "webextension",
                "extension_connected": extension_connected,
                "extension_version": self.extension_version,
                "composer_ready": self.composer_ready,
                "session_ready": session_ready,
                "session_status": session_status,
                "current_url": self.current_url,
                "title": self.title,
                "last_error": self.last_error,
            }

    def take_command(self) -> dict[str, Any] | None:
        with self.condition:
            request = self.pending

            if (
                request is None or
                request.delivered or
                request.completed
            ):
                return None

            request.delivered = True

            return {
                "protocol": EXTENSION_PROTOCOL,
                "command": "send_message",
                "request_id": request.request_id,
                "text": request.text,
            }

    def complete_request(
        self,
        request_id: int,
        response_text: str,
        error_text: str,
    ) -> bool:
        with self.condition:
            request = self.pending

            if (
                request is None or
                request.request_id != request_id or
                request.completed
            ):
                return False

            request.completed = True
            request.response_text = response_text
            request.error_text = error_text
            self.last_error = error_text
            self.condition.notify_all()
            return True

    def submit_message(self, request_id: int, text: str) -> str:
        if request_id < 1:
            raise ValueError("request_id must be positive")

        encoded = text.encode("utf-8")
        if not encoded:
            raise ValueError("message text is empty")
        if len(encoded) > MAX_MESSAGE_BYTES:
            raise ValueError("message text exceeds relay limit")

        with self.condition:
            status = self.get_status_locked()

            if not status["session_ready"]:
                raise RuntimeError(
                    "LibreWolf extension relay is not ready: "
                    + str(status["session_status"])
                )

            if self.pending is not None:
                raise RuntimeError("another browser relay request is already active")

            request = PendingRequest(
                request_id=request_id,
                text=text,
            )
            self.pending = request
            self.condition.notify_all()

            deadline = time.monotonic() + self.response_timeout_seconds

            while not request.completed:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    self.pending = None
                    self.last_error = (
                        "timed out waiting for LibreWolf extension response"
                    )
                    raise TimeoutError(self.last_error)

                self.condition.wait(timeout=min(remaining, 1.0))

            self.pending = None

            if request.error_text:
                raise RuntimeError(request.error_text)

            if not request.response_text:
                raise RuntimeError(
                    "LibreWolf extension returned an empty assistant response"
                )

            return request.response_text

    def get_status_locked(self) -> dict[str, Any]:
        age = (
            time.monotonic() - self.last_heartbeat
            if self.last_heartbeat > 0
            else None
        )

        extension_connected = (
            age is not None and
            age <= HEARTBEAT_STALE_SECONDS
        )

        if not extension_connected:
            session_status = "extension_not_connected"
            session_ready = False
        elif not self.composer_ready:
            session_status = "chatgpt_composer_not_ready"
            session_ready = False
        else:
            session_status = "ready"
            session_ready = True

        return {
            "session_ready": session_ready,
            "session_status": session_status,
        }


class ChatSessionHandler(BaseHTTPRequestHandler):
    server_version = "SalixChatSession/0.2"
    protocol_version = "HTTP/1.0"

    @property
    def state(self) -> RelayState:
        return self.server.state  # type: ignore[attr-defined]

    def _send_json(self, status: int, value: dict[str, Any]) -> None:
        payload = _json_bytes(value)
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(payload)

    def _send_empty(self, status: int) -> None:
        self.send_response(status)
        self.send_header("Content-Length", "0")
        self.send_header("Connection", "close")
        self.end_headers()

    def _read_json(self, maximum_bytes: int) -> dict[str, Any] | None:
        try:
            content_length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self._send_json(
                400,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "bad_request",
                    "error": "invalid Content-Length",
                },
            )
            return None

        if content_length < 1 or content_length > maximum_bytes:
            self._send_json(
                413,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "bad_request",
                    "error": "request body size is invalid",
                },
            )
            return None

        raw = self.rfile.read(content_length)

        try:
            value = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            self._send_json(
                400,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "bad_request",
                    "error": f"invalid JSON: {error}",
                },
            )
            return None

        if not isinstance(value, dict):
            self._send_json(
                400,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "bad_request",
                    "error": "JSON object required",
                },
            )
            return None

        return value

    @staticmethod
    def _validate_extension_protocol(value: dict[str, Any]) -> bool:
        return value.get("protocol") == EXTENSION_PROTOCOL

    def do_GET(self) -> None:  # noqa: N802
        if self.path == "/v1/health":
            self._send_json(200, self.state.get_status())
            return

        if self.path == "/v1/command":
            command = self.state.take_command()
            if command is None:
                self._send_empty(204)
                return

            self._send_json(200, command)
            return

        self._send_json(
            404,
            {
                "protocol": SESSION_PROTOCOL,
                "status": "not_found",
            },
        )

    def do_POST(self) -> None:  # noqa: N802
        if self.path == "/v1/heartbeat":
            request = self._read_json(64 * 1024)
            if request is None:
                return

            if not self._validate_extension_protocol(request):
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": "extension protocol mismatch",
                    },
                )
                return

            self.state.update_heartbeat(
                extension_version=str(request.get("extension_version", "")),
                composer_ready=request.get("composer_ready") is True,
                current_url=str(request.get("current_url", "")),
                title=str(request.get("title", "")),
                error_text=str(request.get("error", "")),
            )
            self._send_json(
                200,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "ok",
                },
            )
            return

        if self.path in ("/v1/result", "/v1/failure"):
            request = self._read_json(MAX_MESSAGE_BYTES * 2)
            if request is None:
                return

            if not self._validate_extension_protocol(request):
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": "extension protocol mismatch",
                    },
                )
                return

            request_id = request.get("request_id")
            if not isinstance(request_id, int) or request_id < 1:
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": "positive integer request_id required",
                    },
                )
                return

            response_text = ""
            error_text = ""

            if self.path == "/v1/result":
                value = request.get("text")
                if not isinstance(value, str) or not value:
                    self._send_json(
                        400,
                        {
                            "protocol": SESSION_PROTOCOL,
                            "status": "bad_request",
                            "error": "non-empty assistant text required",
                        },
                    )
                    return
                response_text = value
            else:
                value = request.get("error")
                if not isinstance(value, str) or not value:
                    value = "LibreWolf extension reported an unspecified failure"
                error_text = value

            if not self.state.complete_request(
                request_id,
                response_text,
                error_text,
            ):
                self._send_json(
                    409,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "stale_request",
                    },
                )
                return

            self._send_json(
                200,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "ok",
                },
            )
            return

        if self.path == "/v1/message":
            request = self._read_json(MAX_MESSAGE_BYTES * 2)
            if request is None:
                return

            request_id = request.get("request_id")
            text = request.get("text")

            if not isinstance(request_id, int) or request_id < 1:
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": "positive integer request_id required",
                    },
                )
                return

            if not isinstance(text, str) or not text:
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": "non-empty text required",
                    },
                )
                return

            print(
                f"[chat-session] request id={request_id} "
                f"text_bytes={len(text.encode('utf-8'))}"
            )

            try:
                response_text = self.state.submit_message(
                    request_id,
                    text,
                )
            except Exception as error:
                detail = f"{type(error).__name__}: {error}"
                print(
                    f"[chat-session] request id={request_id} "
                    f"failed: {detail}"
                )
                self._send_json(
                    502,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "browser_error",
                        "request_id": request_id,
                        "error": detail,
                    },
                )
                return

            print(
                f"[chat-session] request id={request_id} "
                f"response_bytes={len(response_text.encode('utf-8'))}"
            )
            self._send_json(
                200,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "ok",
                    "request_id": request_id,
                    "text": response_text,
                },
            )
            return

        self._send_json(
            404,
            {
                "protocol": SESSION_PROTOCOL,
                "status": "not_found",
            },
        )

    def log_message(self, format: str, *args: object) -> None:
        print(f"[chat-session:{self.client_address[0]}] {format % args}")


class ChatSessionServer(ThreadingHTTPServer):
    def __init__(
        self,
        server_address,
        handler_class,
        state: RelayState,
    ) -> None:
        super().__init__(server_address, handler_class)
        self.state = state


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Run the localhost SalixWeb32 broker used by the LibreWolf "
            "Chat Relay WebExtension."
        )
    )
    parser.add_argument(
        "--listen-host",
        default=DEFAULT_HOST,
        help="worker listen address; keep this at 127.0.0.1",
    )
    parser.add_argument(
        "--listen-port",
        type=int,
        default=DEFAULT_PORT,
        help="localhost worker port (default: 8766)",
    )
    parser.add_argument(
        "--response-timeout",
        type=float,
        default=DEFAULT_RESPONSE_TIMEOUT_SECONDS,
        help="maximum seconds to wait for one ChatGPT response",
    )
    args = parser.parse_args()

    if args.listen_host not in ("127.0.0.1", "localhost"):
        parser.error(
            "the browser worker is intentionally localhost-only; "
            "use 127.0.0.1 or localhost"
        )

    if not 1 <= args.listen_port <= 65535:
        parser.error("--listen-port must be between 1 and 65535")

    if args.response_timeout <= 0:
        parser.error("--response-timeout must be greater than zero")

    state = RelayState(args.response_timeout)

    server = ChatSessionServer(
        (args.listen_host, args.listen_port),
        ChatSessionHandler,
        state,
    )

    print(f"Chat session protocol : {SESSION_PROTOCOL}")
    print(f"Extension protocol    : {EXTENSION_PROTOCOL}")
    print(f"Worker endpoint       : http://{args.listen_host}:{args.listen_port}")
    print("Browser control       : normal LibreWolf WebExtension (no Marionette)")
    print("Authentication        : existing normal LibreWolf profile/session")
    print("Active conversation   : current ChatGPT tab/thread in LibreWolf")
    print("Forwarding            : message text + rendered assistant text only")
    print("Cookies/credentials   : never exposed by this worker API")
    print()
    print("LibreWolf must already be running normally.")
    print("Load tools\\librewolf_chat_relay_extension as a temporary add-on.")
    print("When the extension heartbeat sees the ChatGPT composer, relay is ready.")
    print("Press Ctrl+C here to stop the localhost worker.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping Salix chat session.")
    finally:
        server.server_close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
