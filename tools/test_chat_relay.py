#!/usr/bin/env python3
"""Modern-side smoke test for the SalixWeb32 LibreWolf conversation relay."""

from __future__ import annotations

import argparse
import base64
import http.client
import mimetypes
from pathlib import Path
import sys

CONVERSATION_PROTOCOL = "SALIX-CONVERSATION/1"
ATTACHMENT_PROTOCOL = "SALIX-ATTACHMENT/1"
MAX_ATTACHMENTS = 8
MAX_ATTACHMENT_BYTES = 2 * 1024 * 1024
MAX_TOTAL_ATTACHMENT_BYTES = 4 * 1024 * 1024


def _request(
    host: str,
    port: int,
    method: str,
    path: str,
    body: bytes = b"",
    timeout: float = 240.0,
) -> tuple[int, bytes]:
    connection = http.client.HTTPConnection(host, port, timeout=timeout)
    try:
        headers = {"Connection": "close"}
        if body:
            headers["Content-Type"] = "application/x-salix-conversation; charset=utf-8"
            headers["Content-Length"] = str(len(body))
        connection.request(method, path, body=body, headers=headers)
        response = connection.getresponse()
        return response.status, response.read()
    finally:
        connection.close()


def _parse_attachment_event(payload: bytes) -> tuple[str, str, int]:
    text = payload.decode("ascii", errors="strict")
    lines = text.splitlines()
    if not lines or lines[0] != ATTACHMENT_PROTOCOL:
        raise ValueError("unexpected attachment protocol")

    values: dict[str, str] = {}
    for line in lines[1:]:
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value

    try:
        name = base64.b64decode(
            values.get("name_base64", "").encode("ascii"),
            validate=True,
        ).decode("utf-8", errors="replace")
        data = base64.b64decode(
            values.get("data_base64", "").encode("ascii"),
            validate=True,
        )
        expected = int(values.get("size", "-1"))
    except Exception as error:
        raise ValueError("invalid attachment event payload") from error

    if expected != len(data):
        raise ValueError("attachment event size mismatch")

    return (
        name,
        values.get("mime_type", "application/octet-stream"),
        len(data),
    )


def _parse_events(payload: bytes) -> str:
    header_end = payload.find(b"\n\n")
    if header_end < 0:
        raise ValueError("semantic payload framing is missing")

    metadata_text = payload[:header_end].decode("ascii", errors="strict")
    lines = metadata_text.splitlines()
    if not lines or lines[0] != CONVERSATION_PROTOCOL:
        raise ValueError("unexpected conversation protocol")

    values: dict[str, str] = {}
    for line in lines[1:]:
        if "=" in line:
            key, value = line.split("=", 1)
            values[key] = value

    if values.get("status") != "ok":
        raise ValueError("conversation payload did not report status=ok")

    timing_entries = [
        (key[len("timing_") :], value)
        for key, value in values.items()
        if key.startswith("timing_")
    ]

    if timing_entries:
        print("relay timing:")
        for key, value in sorted(timing_entries):
            print(f"  {key}={value} ms")
        print()

    try:
        event_count = int(values.get("event_count", "0"))
    except ValueError as error:
        raise ValueError("invalid event_count") from error

    cursor = header_end + 2
    text_parts: list[str] = []

    for index in range(event_count):
        event_type = values.get(f"event_{index}_type", "")
        try:
            length = int(values.get(f"event_{index}_len", "-1"))
        except ValueError as error:
            raise ValueError(f"invalid event_{index}_len") from error

        if length < 0 or cursor + length > len(payload):
            raise ValueError(f"event {index} exceeds payload")

        event_text = payload[cursor : cursor + length]
        cursor += length

        print(f"event[{index}] {event_type} bytes={length}")

        if event_type == "text_delta":
            text_parts.append(event_text.decode("utf-8", errors="replace"))
        elif event_type == "attachment":
            name, mime_type, size = _parse_attachment_event(event_text)
            print(
                f"  attachment name={name!r} mime={mime_type!r} bytes={size}"
            )

    if cursor != len(payload):
        raise ValueError("trailing bytes remain after event parsing")

    return "".join(text_parts)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check the bridge + LibreWolf extension broker before the P4 test."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument(
        "--message",
        default=None,
        help="optional real message to send through the open ChatGPT conversation",
    )
    parser.add_argument(
        "--file",
        action="append",
        default=[],
        help="optional file attachment; may be supplied up to 8 times",
    )
    args = parser.parse_args()

    status, health = _request(
        args.host,
        args.port,
        "GET",
        "/v1/health",
        timeout=5.0,
    )
    print(f"health HTTP {status}")
    print(health.decode("utf-8", errors="replace").rstrip())
    print()

    if status != 200:
        return 1

    health_text = health.decode("utf-8", errors="replace")
    ready_marker = "conversation_browser_session=ready"

    if ready_marker not in health_text:
        if "conversation_browser_session=worker_unavailable" in health_text:
            detail = (
                "The localhost chat-session broker is not running. "
                "Start 'python tools\\salix_chat_session.py' and leave it running."
            )
        elif "conversation_browser_session=extension_not_connected" in health_text:
            detail = (
                "The broker is running but the LibreWolf relay extension is not "
                "connected. In normal LibreWolf open "
                "'about:debugging#/runtime/this-firefox', load "
                "'tools\\librewolf_chat_relay_extension\\manifest.json', "
                "then reload the ChatGPT tab."
            )
        elif "conversation_browser_session=chatgpt_composer_not_ready" in health_text:
            detail = (
                "The LibreWolf extension is connected, but it cannot see a usable "
                "ChatGPT composer. Open the desired ChatGPT thread in normal "
                "LibreWolf and reload that tab."
            )
        else:
            detail = (
                "The browser relay is not ready. Review "
                "conversation_browser_session in the health output above."
            )

        print("ERROR: " + detail, file=sys.stderr)
        return 2

    if args.message is None and not args.file:
        print("browser relay ready")
        return 0

    if len(args.file) > MAX_ATTACHMENTS:
        print("ERROR: at most 8 --file arguments are supported", file=sys.stderr)
        return 2

    encoded = (args.message or "").encode("utf-8")
    attachments: list[tuple[bytes, bytes, bytes]] = []
    total_attachment_bytes = 0

    for value in args.file:
        path = Path(value)
        try:
            data = path.read_bytes()
        except OSError as error:
            print(f"ERROR: could not read {path}: {error}", file=sys.stderr)
            return 2

        if len(data) > MAX_ATTACHMENT_BYTES:
            print(f"ERROR: {path.name} exceeds 2 MB", file=sys.stderr)
            return 2

        total_attachment_bytes += len(data)
        if total_attachment_bytes > MAX_TOTAL_ATTACHMENT_BYTES:
            print("ERROR: attachments exceed 4 MB total", file=sys.stderr)
            return 2

        mime_type = (
            mimetypes.guess_type(path.name)[0] or
            "application/octet-stream"
        )
        attachments.append(
            (
                path.name.encode("utf-8"),
                mime_type.encode("ascii", errors="replace"),
                data,
            )
        )

    lines = [
        CONVERSATION_PROTOCOL,
        "mode=browser_relay",
        "request_id=1",
        f"text_forwarded={1 if encoded else 0}",
        f"attachments_forwarded={1 if attachments else 0}",
        "credentials_forwarded=0",
        "session_forwarded=0",
        f"text_len={len(encoded)}",
        f"attachment_count={len(attachments)}",
    ]

    for index, (name, mime_type, data) in enumerate(attachments):
        lines.extend(
            [
                f"attachment_{index}_name_len={len(name)}",
                f"attachment_{index}_mime_len={len(mime_type)}",
                f"attachment_{index}_data_len={len(data)}",
            ]
        )

    body = ("\n".join(lines) + "\n\n").encode("ascii") + encoded
    for name, mime_type, data in attachments:
        body += name + mime_type + data

    print("sending real browser-relay message...")
    status, payload = _request(
        args.host,
        args.port,
        "POST",
        "/v1/conversation/message",
        body=body,
    )
    print(f"message HTTP {status}")

    if status != 200:
        print(payload.decode("utf-8", errors="replace"))
        return 1

    try:
        response_text = _parse_events(payload)
    except ValueError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print()
    print("assistant response:")
    print("-------------------")
    print(response_text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
