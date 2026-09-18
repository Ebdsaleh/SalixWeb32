#!/usr/bin/env python3
"""Modern-side smoke test for the SalixWeb32 LibreWolf conversation relay."""

from __future__ import annotations

import argparse
import http.client
import sys

CONVERSATION_PROTOCOL = "SALIX-CONVERSATION/1"


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

    if cursor != len(payload):
        raise ValueError("trailing bytes remain after event parsing")

    return "".join(text_parts)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check salix_bridge.py + LibreWolf worker before the P4 test."
    )
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument(
        "--message",
        default=None,
        help="optional real message to send through the open ChatGPT conversation",
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

    if args.message is None:
        return 0

    encoded = args.message.encode("utf-8")
    body = (
        f"{CONVERSATION_PROTOCOL}\n"
        "mode=browser_relay\n"
        "request_id=1\n"
        "text_forwarded=1\n"
        "attachments_forwarded=0\n"
        "credentials_forwarded=0\n"
        "session_forwarded=0\n"
        f"text_len={len(encoded)}\n"
        "\n"
    ).encode("ascii") + encoded

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
