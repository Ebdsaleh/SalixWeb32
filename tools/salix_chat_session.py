#!/usr/bin/env python3
"""Local broker between the SalixWeb32 bridge and a normal LibreWolf extension.

This process does NOT launch or automate LibreWolf.

The user's ordinary LibreWolf process owns the authenticated ChatGPT session.
A small WebExtension loaded into that browser communicates with this localhost-only
broker. The P4-facing salix_bridge.py remains a separate process.
"""

from __future__ import annotations

import argparse
import base64
import json
import mimetypes
from pathlib import Path
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
MAX_RELAY_JSON_BYTES = 8 * 1024 * 1024
MAX_ATTACHMENTS = 8
MAX_ATTACHMENT_BYTES = 2 * 1024 * 1024
MAX_TOTAL_ATTACHMENT_BYTES = 4 * 1024 * 1024
HEARTBEAT_STALE_SECONDS = 4.0


def _json_bytes(value: dict[str, Any]) -> bytes:
    return json.dumps(value, ensure_ascii=False).encode("utf-8")


TIMING_KEYS = (
    "browser_submit_ms",
    "browser_first_response_ms",
    "browser_generation_ms",
    "browser_stabilization_ms",
    "browser_total_ms",
    "background_total_ms",
)


def _sanitize_timing(value: Any) -> dict[str, int]:
    if not isinstance(value, dict):
        return {}

    timing: dict[str, int] = {}

    for key in TIMING_KEYS:
        raw = value.get(key)

        if isinstance(raw, bool):
            continue

        if isinstance(raw, (int, float)):
            milliseconds = int(round(raw))
            if 0 <= milliseconds <= 24 * 60 * 60 * 1000:
                timing[key] = milliseconds

    return timing


ATTACHMENT_DEBUG_INTEGER_KEYS = (
    "scan_passes",
    "candidates_seen",
    "sandbox_candidates",
    "download_capture_attempts",
    "download_capture_successes",
    "download_url_candidates",
    "intercept_capture_attempts",
    "intercept_capture_successes",
    "intercepted_requests",
    "managed_download_successes",
    "direct_fetch_attempts",
    "direct_fetch_successes",
    "preview_open_attempts",
    "preview_download_controls",
    "preview_close_successes",
    "attachments_collected",
)


def _sanitize_attachment_debug(value: Any) -> dict[str, Any]:
    if not isinstance(value, dict):
        return {}

    debug: dict[str, Any] = {}

    scan_root = value.get("scan_root")
    if isinstance(scan_root, str):
        debug["scan_root"] = scan_root[:128]

    for key in ATTACHMENT_DEBUG_INTEGER_KEYS:
        raw = value.get(key)
        if isinstance(raw, bool):
            continue
        if isinstance(raw, int) and 0 <= raw <= 10000:
            debug[key] = raw

    errors = value.get("errors")
    if isinstance(errors, list):
        debug["errors"] = [
            str(item)[:240]
            for item in errors[:8]
            if isinstance(item, (str, int, float))
        ]

    return debug


def _safe_attachment_name(value: str) -> str:
    value = value.replace("\\", "/").split("/")[-1].strip()
    value = value.replace("\r", "_").replace("\n", "_")
    if not value or value in (".", ".."):
        return "attachment.bin"
    return value[:255]


def _normalize_attachments(value: Any) -> list[dict[str, str]]:
    if value is None:
        return []
    if not isinstance(value, list):
        raise ValueError("attachments must be an array")
    if len(value) > MAX_ATTACHMENTS:
        raise ValueError("too many attachments")

    attachments: list[dict[str, str]] = []
    total_bytes = 0

    for item in value:
        if not isinstance(item, dict):
            raise ValueError("attachment entry is invalid")

        name = item.get("name")
        mime_type = item.get("mime_type", "application/octet-stream")
        encoded = item.get("data_base64")

        if not isinstance(name, str) or not name:
            raise ValueError("attachment name is invalid")
        if not isinstance(mime_type, str) or not mime_type:
            mime_type = "application/octet-stream"
        if not isinstance(encoded, str):
            raise ValueError("attachment data is missing")

        try:
            raw = base64.b64decode(encoded.encode("ascii"), validate=True)
        except Exception as error:
            raise ValueError("attachment base64 is invalid") from error

        if len(raw) > MAX_ATTACHMENT_BYTES:
            raise ValueError("attachment exceeds 2 MB limit")

        total_bytes += len(raw)
        if total_bytes > MAX_TOTAL_ATTACHMENT_BYTES:
            raise ValueError("attachments exceed 4 MB total limit")

        attachments.append(
            {
                "name": _safe_attachment_name(name),
                "mime_type": mime_type[:128],
                "data_base64": encoded,
            }
        )

    return attachments


def _normalize_extension_response_attachments(
    value: Any,
) -> list[dict[str, str]]:
    if value is None:
        return []
    if not isinstance(value, list):
        raise ValueError("attachments must be an array")
    if len(value) > MAX_ATTACHMENTS:
        raise ValueError("too many attachments")

    attachments: list[dict[str, str]] = []
    total_bytes = 0

    for item in value:
        if not isinstance(item, dict):
            raise ValueError("attachment entry is invalid")

        name = item.get("name")
        mime_type = item.get("mime_type", "application/octet-stream")
        encoded = item.get("data_base64")
        local_path = item.get("local_path")

        if not isinstance(name, str) or not name:
            raise ValueError("attachment name is invalid")
        if not isinstance(mime_type, str) or not mime_type:
            mime_type = "application/octet-stream"

        if isinstance(encoded, str):
            try:
                raw = base64.b64decode(
                    encoded.encode("ascii"),
                    validate=True,
                )
            except Exception as error:
                raise ValueError(
                    "attachment base64 is invalid"
                ) from error
        elif isinstance(local_path, str) and local_path:
            path = Path(local_path)

            try:
                size = path.stat().st_size
            except OSError as error:
                raise ValueError(
                    "captured attachment file is unavailable"
                ) from error

            if size < 0 or size > MAX_ATTACHMENT_BYTES:
                raise ValueError(
                    "captured attachment exceeds 2 MB limit"
                )

            try:
                raw = path.read_bytes()
            except OSError as error:
                raise ValueError(
                    "captured attachment could not be read"
                ) from error

            encoded = base64.b64encode(raw).decode("ascii")

            if (
                mime_type == "application/octet-stream" and
                path.name
            ):
                guessed = mimetypes.guess_type(path.name)[0]
                if guessed:
                    mime_type = guessed
        else:
            raise ValueError("attachment data is missing")

        if len(raw) > MAX_ATTACHMENT_BYTES:
            raise ValueError("attachment exceeds 2 MB limit")

        total_bytes += len(raw)
        if total_bytes > MAX_TOTAL_ATTACHMENT_BYTES:
            raise ValueError(
                "attachments exceed 4 MB total limit"
            )

        attachments.append(
            {
                "name": _safe_attachment_name(name),
                "mime_type": mime_type[:128],
                "data_base64": encoded,
            }
        )

    return attachments


@dataclass
class PendingRequest:
    request_id: int
    text: str
    attachments: list[dict[str, str]]
    created_at: float
    delivered_at: float = 0.0
    completed_at: float = 0.0
    delivered: bool = False
    completed: bool = False
    response_text: str = ""
    response_attachments: list[dict[str, str]] | None = None
    error_text: str = ""
    extension_timing: dict[str, int] | None = None


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
            request.delivered_at = time.monotonic()

            return {
                "protocol": EXTENSION_PROTOCOL,
                "command": "send_message",
                "request_id": request.request_id,
                "text": request.text,
                "attachments": request.attachments,
            }

    def complete_request(
        self,
        request_id: int,
        response_text: str,
        response_attachments: list[dict[str, str]],
        error_text: str,
        extension_timing: dict[str, int] | None = None,
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
            request.completed_at = time.monotonic()
            request.response_text = response_text
            request.response_attachments = response_attachments
            request.error_text = error_text
            request.extension_timing = extension_timing or {}
            self.last_error = error_text
            self.condition.notify_all()
            return True

    def submit_message(
        self,
        request_id: int,
        text: str,
        attachments: list[dict[str, str]],
    ) -> tuple[str, list[dict[str, str]], dict[str, int]]:
        if request_id < 1:
            raise ValueError("request_id must be positive")

        encoded = text.encode("utf-8")
        if len(encoded) > MAX_MESSAGE_BYTES:
            raise ValueError("message text exceeds relay limit")
        if not encoded and not attachments:
            raise ValueError("message text or attachment is required")

        normalized_attachments = _normalize_attachments(attachments)

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
                attachments=normalized_attachments,
                created_at=time.monotonic(),
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

            response_attachments = request.response_attachments or []
            if not request.response_text and not response_attachments:
                raise RuntimeError(
                    "LibreWolf extension returned an empty assistant response"
                )

            delivered_at = (
                request.delivered_at
                if request.delivered_at > 0
                else request.created_at
            )
            completed_at = (
                request.completed_at
                if request.completed_at > 0
                else time.monotonic()
            )

            timing = dict(request.extension_timing or {})
            timing["broker_queue_ms"] = max(
                0,
                int(round((delivered_at - request.created_at) * 1000.0)),
            )
            timing["broker_extension_ms"] = max(
                0,
                int(round((completed_at - delivered_at) * 1000.0)),
            )
            timing["broker_total_ms"] = max(
                0,
                int(round((completed_at - request.created_at) * 1000.0)),
            )

            return request.response_text, response_attachments, timing

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

    def _send_cors_headers(self) -> None:
        # The broker is bound to loopback only and never uses browser credentials.
        # Allow the local WebExtension origin to make JSON requests through the
        # normal browser CORS preflight path.
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header(
            "Access-Control-Allow-Methods",
            "GET, POST, OPTIONS",
        )
        self.send_header(
            "Access-Control-Allow-Headers",
            "Content-Type",
        )
        self.send_header("Access-Control-Max-Age", "600")

    def _send_json(self, status: int, value: dict[str, Any]) -> None:
        payload = _json_bytes(value)
        self.send_response(status)
        self._send_cors_headers()
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(payload)

    def _send_empty(self, status: int) -> None:
        self.send_response(status)
        self._send_cors_headers()
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

    def do_OPTIONS(self) -> None:  # noqa: N802
        if self.path not in (
            "/v1/health",
            "/v1/command",
            "/v1/heartbeat",
            "/v1/result",
            "/v1/failure",
            "/v1/message",
        ):
            self._send_empty(404)
            return

        self._send_empty(204)

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
            request = self._read_json(MAX_RELAY_JSON_BYTES)
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
            response_attachments: list[dict[str, str]] = []
            error_text = ""

            if self.path == "/v1/result":
                value = request.get("text", "")
                if not isinstance(value, str):
                    self._send_json(
                        400,
                        {
                            "protocol": SESSION_PROTOCOL,
                            "status": "bad_request",
                            "error": "assistant text must be a string",
                        },
                    )
                    return
                response_text = value

                attachment_debug = _sanitize_attachment_debug(
                    request.get("attachment_debug")
                )

                if attachment_debug:
                    print(
                        "[chat-session] attachment capture "
                        + " ".join(
                            f"{key}={attachment_debug[key]!r}"
                            for key in sorted(attachment_debug)
                        )
                    )

                try:
                    response_attachments = (
                        _normalize_extension_response_attachments(
                            request.get("attachments")
                        )
                    )
                except ValueError as error:
                    self._send_json(
                        400,
                        {
                            "protocol": SESSION_PROTOCOL,
                            "status": "bad_request",
                            "error": str(error),
                        },
                    )
                    return

                if not response_text and not response_attachments:
                    self._send_json(
                        400,
                        {
                            "protocol": SESSION_PROTOCOL,
                            "status": "bad_request",
                            "error": "assistant response is empty",
                        },
                    )
                    return
            else:
                value = request.get("error")
                if not isinstance(value, str) or not value:
                    value = "LibreWolf extension reported an unspecified failure"
                error_text = value

            extension_timing = (
                _sanitize_timing(request.get("timing"))
                if self.path == "/v1/result"
                else {}
            )

            if not self.state.complete_request(
                request_id,
                response_text,
                response_attachments,
                error_text,
                extension_timing,
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
            request = self._read_json(MAX_RELAY_JSON_BYTES)
            if request is None:
                return

            request_id = request.get("request_id")
            text = request.get("text", "")

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

            if not isinstance(text, str):
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": "text must be a string",
                    },
                )
                return

            try:
                attachments = _normalize_attachments(request.get("attachments"))
            except ValueError as error:
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": str(error),
                    },
                )
                return

            if not text and not attachments:
                self._send_json(
                    400,
                    {
                        "protocol": SESSION_PROTOCOL,
                        "status": "bad_request",
                        "error": "text or attachment is required",
                    },
                )
                return

            print(
                f"[chat-session] request id={request_id} "
                f"text_bytes={len(text.encode('utf-8'))} "
                f"attachments={len(attachments)}"
            )

            try:
                response_text, response_attachments, relay_timing = (
                    self.state.submit_message(
                        request_id,
                        text,
                        attachments,
                    )
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
                f"response_bytes={len(response_text.encode('utf-8'))} "
                f"attachments={len(response_attachments)}"
            )
            print(
                "[chat-session] timing "
                + " ".join(
                    f"{key}={relay_timing[key]}ms"
                    for key in sorted(relay_timing)
                )
            )
            self._send_json(
                200,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "ok",
                    "request_id": request_id,
                    "text": response_text,
                    "attachments": response_attachments,
                    "timing": relay_timing,
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
        rendered = format % args

        # Heartbeats and empty command polls are expected high-frequency traffic.
        # Keep the console useful by hiding their successful noise while still
        # printing requests that carry work or report errors.
        if (
            '"GET /v1/command HTTP/' in rendered and
            " 204 " in rendered
        ):
            return

        if (
            '"OPTIONS /v1/heartbeat HTTP/' in rendered and
            " 204 " in rendered
        ):
            return

        if (
            '"POST /v1/heartbeat HTTP/' in rendered and
            " 200 " in rendered
        ):
            return

        print(f"[chat-session:{self.client_address[0]}] {rendered}")


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
        help="broker listen address; keep this at 127.0.0.1",
    )
    parser.add_argument(
        "--listen-port",
        type=int,
        default=DEFAULT_PORT,
        help="localhost broker port (default: 8766)",
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
            "the chat-session broker is intentionally localhost-only; "
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
    print(f"Broker endpoint       : http://{args.listen_host}:{args.listen_port}")
    print("Browser control       : normal LibreWolf WebExtension (no Marionette)")
    print("Authentication        : existing normal LibreWolf profile/session")
    print("Active conversation   : current ChatGPT tab/thread in LibreWolf")
    print("Forwarding            : message text + bounded files + assistant files")
    print("Cookies/credentials   : never exposed by this broker API")
    print()
    print("LibreWolf must already be running normally.")
    print("Load tools\\librewolf_chat_relay_extension as a temporary add-on.")
    print("When the extension heartbeat sees the ChatGPT composer, relay is ready.")
    print("Press Ctrl+C here to stop the localhost broker.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping Salix chat session.")
    finally:
        server.server_close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
