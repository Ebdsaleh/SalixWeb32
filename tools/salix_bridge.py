#!/usr/bin/env python3
"""Minimal modern-side companion for the first SalixWeb32 bridge tranche.

This server intentionally does not contact ChatGPT or any other Internet service.
It proves that the legacy client can cross the LAN boundary into a modern process
through the generic NetworkTransport / RemoteBridgeWebBackend architecture.
"""

from __future__ import annotations

import argparse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PROTOCOL = "SALIX-BRIDGE/1"


class SalixBridgeHandler(BaseHTTPRequestHandler):
    server_version = "SalixBridge/0.1"
    protocol_version = "HTTP/1.0"

    def _send_text(self, status: int, body: str) -> None:
        payload = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self) -> None:  # noqa: N802 - BaseHTTPRequestHandler contract
        if self.path == "/v1/health":
            self._send_text(
                200,
                f"{PROTOCOL}\nstatus=ok\nservice=salix_bridge\n",
            )
            return

        self._send_text(
            404,
            f"{PROTOCOL}\nstatus=not_found\npath={self.path}\n",
        )

    def do_POST(self) -> None:  # noqa: N802 - BaseHTTPRequestHandler contract
        if self.path != "/v1/navigate":
            self._send_text(
                404,
                f"{PROTOCOL}\nstatus=not_found\npath={self.path}\n",
            )
            return

        try:
            content_length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self._send_text(
                400,
                f"{PROTOCOL}\nstatus=bad_content_length\n",
            )
            return

        if content_length < 0 or content_length > 1024 * 1024:
            self._send_text(
                413,
                f"{PROTOCOL}\nstatus=payload_too_large\n",
            )
            return

        raw_body = self.rfile.read(content_length)
        target = raw_body.decode("utf-8", errors="replace").strip()

        if not target:
            self._send_text(
                400,
                f"{PROTOCOL}\nstatus=missing_target\n",
            )
            return

        self._send_text(
            200,
            f"{PROTOCOL}\n"
            "status=accepted\n"
            f"target={target}\n"
            "note=transport boundary verified; modern fetch is not enabled yet\n",
        )

    def log_message(self, format: str, *args: object) -> None:
        print(f"[{self.client_address[0]}] {format % args}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run the minimal SalixWeb32 modern-side bridge companion."
    )
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="listen address (use 0.0.0.0 only on a trusted LAN)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=8765,
        help="listen port (default: 8765)",
    )
    args = parser.parse_args()

    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535")

    server = ThreadingHTTPServer((args.host, args.port), SalixBridgeHandler)

    print(f"Salix bridge protocol : {PROTOCOL}")
    print(f"Listening             : http://{args.host}:{args.port}")
    print("Internet/service I/O  : disabled in this tranche")
    print("Press Ctrl+C to stop.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping Salix bridge.")
    finally:
        server.server_close()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
