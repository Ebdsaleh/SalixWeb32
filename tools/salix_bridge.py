#!/usr/bin/env python3
"""Modern-side companion for SalixWeb32 bridge and Browser Probe development.

The bridge keeps modern HTTPS/TLS work off the Windows Server 2003 target while
SalixWeb32 learns what a modern URL actually returns. Browser Probe requests are
intentionally unauthenticated and never forward cookies or credentials.
"""

from __future__ import annotations

import argparse
import http.client
import re
import socket
import urllib.error
import urllib.parse
import urllib.request
from html.parser import HTMLParser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PROTOCOL = "SALIX-BRIDGE/1"
PROBE_PROTOCOL = "SALIX-PROBE/1"
MAX_REQUEST_BYTES = 1024 * 1024
MAX_FETCH_BYTES = 512 * 1024
MAX_EXTRACTED_BYTES = 64 * 1024
FETCH_TIMEOUT_SECONDS = 10


class ProbeHtmlParser(HTMLParser):
    """Extract small diagnostic facts without pretending to be a browser DOM."""

    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self._skip_depth = 0
        self._in_title = False
        self._title_parts: list[str] = []
        self._text_parts: list[str] = []
        self.script_count = 0
        self.form_count = 0
        self.link_count = 0

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        del attrs
        lowered = tag.lower()
        if lowered == "script":
            self.script_count += 1
            self._skip_depth += 1
        elif lowered in ("style", "noscript", "svg"):
            self._skip_depth += 1
        elif lowered == "form":
            self.form_count += 1
        elif lowered == "a":
            self.link_count += 1
        elif lowered == "title":
            self._in_title = True

    def handle_endtag(self, tag: str) -> None:
        lowered = tag.lower()
        if lowered in ("script", "style", "noscript", "svg"):
            if self._skip_depth > 0:
                self._skip_depth -= 1
        elif lowered == "title":
            self._in_title = False

    def handle_data(self, data: str) -> None:
        if self._in_title:
            self._title_parts.append(data)
        if self._skip_depth == 0:
            stripped = data.strip()
            if stripped:
                self._text_parts.append(stripped)

    def get_title(self) -> str:
        return " ".join(" ".join(self._title_parts).split())

    def get_text(self) -> str:
        return "\n".join(self._text_parts)


class CountingRedirectHandler(urllib.request.HTTPRedirectHandler):
    def __init__(self) -> None:
        super().__init__()
        self.redirect_count = 0

    def redirect_request(self, req, fp, code, msg, headers, newurl):  # type: ignore[no-untyped-def]
        self.redirect_count += 1
        return super().redirect_request(req, fp, code, msg, headers, newurl)


def _create_ipv4_connection(
    address,
    timeout=socket._GLOBAL_DEFAULT_TIMEOUT,  # type: ignore[attr-defined]
    source_address=None,
):  # type: ignore[no-untyped-def]
    """Create one TCP connection using IPv4 only.

    The Server 2022 probe host used for target validation has a healthy IPv4
    path to modern services while its IPv6 resolver/connect path is unusable.
    Keeping the diagnostic fetch deterministic avoids burning the complete
    probe timeout on an address family that cannot succeed on that host.
    """

    host, port = address
    last_error: OSError | None = None

    for family, socktype, proto, _canonname, socket_address in socket.getaddrinfo(
        host,
        port,
        socket.AF_INET,
        socket.SOCK_STREAM,
    ):
        sock = socket.socket(family, socktype, proto)
        try:
            if timeout is not socket._GLOBAL_DEFAULT_TIMEOUT:  # type: ignore[attr-defined]
                sock.settimeout(timeout)
            if source_address:
                sock.bind(source_address)
            sock.connect(socket_address)
            return sock
        except OSError as error:
            last_error = error
            sock.close()

    if last_error is not None:
        raise last_error
    raise OSError("IPv4 address resolution returned no usable addresses")


class IPv4HTTPConnection(http.client.HTTPConnection):
    _create_connection = staticmethod(_create_ipv4_connection)


class IPv4HTTPSConnection(http.client.HTTPSConnection):
    _create_connection = staticmethod(_create_ipv4_connection)


class IPv4HTTPHandler(urllib.request.HTTPHandler):
    def http_open(self, request):  # type: ignore[no-untyped-def]
        return self.do_open(IPv4HTTPConnection, request)


class IPv4HTTPSHandler(urllib.request.HTTPSHandler):
    def https_open(self, request):  # type: ignore[no-untyped-def]
        return self.do_open(
            IPv4HTTPSConnection,
            request,
            context=self._context,
            check_hostname=self._check_hostname,
        )


def _safe_headers(headers) -> str:  # type: ignore[no-untyped-def]
    """Render response headers while keeping session material off plaintext LAN."""

    lines: list[str] = []
    for name, value in headers.items():
        lowered = name.lower()
        if lowered in ("set-cookie", "authorization", "proxy-authorization"):
            value = "[redacted by Salix Browser Probe]"
        lines.append(f"{name}: {value}")
    return "\r\n".join(lines)


def _decode_body(raw: bytes, charset: str | None) -> str:
    encoding = charset or "utf-8"
    try:
        return raw.decode(encoding, errors="replace")
    except LookupError:
        return raw.decode("utf-8", errors="replace")


def _limit_utf8(text: str, maximum_bytes: int) -> str:
    encoded = text.encode("utf-8")
    if len(encoded) <= maximum_bytes:
        return text
    return encoded[:maximum_bytes].decode("utf-8", errors="ignore")


def _frame_probe_result(
    *,
    requested_url: str,
    final_url: str,
    http_status: int,
    http_reason: str,
    mime_type: str,
    response_size: int,
    truncated: bool,
    redirect_count: int,
    headers: str,
    raw_text: str,
    extracted_text: str,
    title: str,
    script_count: int,
    form_count: int,
    link_count: int,
) -> bytes:
    sections = [
        requested_url,
        final_url,
        http_reason,
        mime_type,
        headers,
        raw_text,
        extracted_text,
        title,
    ]
    encoded_sections = [section.encode("utf-8") for section in sections]
    names = [
        "requested_url_len",
        "final_url_len",
        "http_reason_len",
        "mime_type_len",
        "headers_len",
        "raw_len",
        "extracted_len",
        "title_len",
    ]

    lines = [
        PROBE_PROTOCOL,
        "status=ok",
        "address_family=ipv4",
        f"http_status={http_status}",
        f"response_size={response_size}",
        f"truncated={1 if truncated else 0}",
        f"redirect_count={redirect_count}",
        f"script_count={script_count}",
        f"form_count={form_count}",
        f"link_count={link_count}",
    ]
    for name, section in zip(names, encoded_sections):
        lines.append(f"{name}={len(section)}")

    header = ("\n".join(lines) + "\n\n").encode("ascii")
    return header + b"".join(encoded_sections)


def _perform_probe(target: str) -> bytes:
    parsed = urllib.parse.urlparse(target)
    if parsed.scheme.lower() not in ("http", "https") or not parsed.netloc:
        raise ValueError("Browser Probe accepts only absolute http:// or https:// URLs")

    redirect_handler = CountingRedirectHandler()
    opener = urllib.request.build_opener(
        redirect_handler,
        IPv4HTTPHandler(),
        IPv4HTTPSHandler(),
    )
    request = urllib.request.Request(
        target,
        method="GET",
        headers={
            "User-Agent": "SalixWeb32-BrowserProbe/0.1",
            "Accept": "text/html,application/xhtml+xml,application/json,text/plain,*/*;q=0.5",
            "Accept-Encoding": "identity",
            "Connection": "close",
        },
    )

    response = None
    try:
        try:
            response = opener.open(request, timeout=FETCH_TIMEOUT_SECONDS)
        except urllib.error.HTTPError as error:
            response = error

        raw = response.read(MAX_FETCH_BYTES + 1)
        truncated = len(raw) > MAX_FETCH_BYTES
        if truncated:
            raw = raw[:MAX_FETCH_BYTES]

        status = int(getattr(response, "status", response.getcode() or 0))
        reason = str(getattr(response, "reason", ""))
        final_url = response.geturl()
        mime_type = response.headers.get_content_type() or "application/octet-stream"
        charset = response.headers.get_content_charset()
        raw_text = _decode_body(raw, charset)
        headers_text = _safe_headers(response.headers)

        title = ""
        extracted_text = ""
        script_count = 0
        form_count = 0
        link_count = 0

        if mime_type in ("text/html", "application/xhtml+xml"):
            parser = ProbeHtmlParser()
            try:
                parser.feed(raw_text)
                parser.close()
            except Exception:
                # Raw response inspection remains useful even when malformed HTML
                # defeats the deliberately tiny diagnostic extractor.
                pass
            title = parser.get_title()
            extracted_text = parser.get_text()
            script_count = parser.script_count
            form_count = parser.form_count
            link_count = parser.link_count
        elif mime_type.startswith("text/") or mime_type in (
            "application/json",
            "application/javascript",
        ):
            extracted_text = raw_text

        extracted_text = _limit_utf8(extracted_text, MAX_EXTRACTED_BYTES)

        return _frame_probe_result(
            requested_url=target,
            final_url=final_url,
            http_status=status,
            http_reason=reason,
            mime_type=mime_type,
            response_size=len(raw),
            truncated=truncated,
            redirect_count=redirect_handler.redirect_count,
            headers=headers_text,
            raw_text=raw_text,
            extracted_text=extracted_text,
            title=title,
            script_count=script_count,
            form_count=form_count,
            link_count=link_count,
        )
    finally:
        if response is not None:
            response.close()


class SalixBridgeHandler(BaseHTTPRequestHandler):
    server_version = "SalixBridge/0.2"
    protocol_version = "HTTP/1.0"

    def _send_bytes(self, status: int, payload: bytes, content_type: str) -> None:
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(payload)

    def _send_text(self, status: int, body: str) -> None:
        self._send_bytes(
            status,
            body.encode("utf-8"),
            "text/plain; charset=utf-8",
        )

    def _read_request_body(self) -> bytes | None:
        try:
            content_length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self._send_text(
                400,
                f"{PROTOCOL}\nstatus=bad_content_length\n",
            )
            return None

        if content_length < 0 or content_length > MAX_REQUEST_BYTES:
            self._send_text(
                413,
                f"{PROTOCOL}\nstatus=payload_too_large\n",
            )
            return None

        return self.rfile.read(content_length)

    def do_GET(self) -> None:  # noqa: N802 - BaseHTTPRequestHandler contract
        if self.path == "/v1/health":
            self._send_text(
                200,
                f"{PROTOCOL}\nstatus=ok\nservice=salix_bridge\nprobe=enabled\n",
            )
            return

        self._send_text(
            404,
            f"{PROTOCOL}\nstatus=not_found\npath={self.path}\n",
        )

    def do_POST(self) -> None:  # noqa: N802 - BaseHTTPRequestHandler contract
        if self.path not in ("/v1/navigate", "/v1/fetch"):
            self._send_text(
                404,
                f"{PROTOCOL}\nstatus=not_found\npath={self.path}\n",
            )
            return

        raw_body = self._read_request_body()
        if raw_body is None:
            return

        target = raw_body.decode("utf-8", errors="replace").strip()
        if not target:
            self._send_text(
                400,
                f"{PROTOCOL}\nstatus=missing_target\n",
            )
            return

        if self.path == "/v1/navigate":
            self._send_text(
                200,
                f"{PROTOCOL}\n"
                "status=accepted\n"
                f"target={target}\n"
                "note=transport boundary verified\n",
            )
            return

        try:
            payload = _perform_probe(target)
        except ValueError as error:
            self._send_text(
                400,
                f"{PROBE_PROTOCOL}\nstatus=invalid_target\nerror={error}\n",
            )
            return
        except urllib.error.URLError as error:
            self._send_text(
                502,
                f"{PROBE_PROTOCOL}\nstatus=fetch_failed\nerror={error.reason}\n",
            )
            return
        except TimeoutError:
            self._send_text(
                504,
                f"{PROBE_PROTOCOL}\nstatus=fetch_timeout\n",
            )
            return
        except Exception as error:
            self._send_text(
                502,
                f"{PROBE_PROTOCOL}\nstatus=fetch_failed\n"
                f"error={type(error).__name__}: {error}\n",
            )
            return

        self._send_bytes(
            200,
            payload,
            "application/x-salix-probe",
        )

    def log_message(self, format: str, *args: object) -> None:
        print(f"[{self.client_address[0]}] {format % args}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Run the SalixWeb32 modern-side bridge companion."
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
    print(f"Browser probe protocol: {PROBE_PROTOCOL}")
    print(f"Listening             : http://{args.host}:{args.port}")
    print("Modern HTTPS probe    : enabled (unauthenticated GET only)")
    print("Probe address family  : IPv4")
    print("Credentials/cookies   : never forwarded by Browser Probe")
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
