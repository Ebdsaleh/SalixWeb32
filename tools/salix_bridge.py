#!/usr/bin/env python3
"""Modern-side listener/broker for SalixWeb32 compatibility work.

Browser Probe remains a bounded diagnostic HTTPS fetch. Conversation traffic can use
the separate localhost-only salix_chat_session.py worker, which owns the visible
LibreWolf/ChatGPT session. The bridge never receives ChatGPT credentials, cookies, or
browser session storage; only user message text and rendered assistant response text
cross the trusted development LAN.
"""

from __future__ import annotations

import argparse
import http.client
import json
import re
import socket
import urllib.error
import urllib.parse
import urllib.request
from html.parser import HTMLParser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PROTOCOL = "SALIX-BRIDGE/1"
PROBE_PROTOCOL = "SALIX-PROBE/1"
CONVERSATION_PROTOCOL = "SALIX-CONVERSATION/1"
MAX_REQUEST_BYTES = 1024 * 1024
MAX_FETCH_BYTES = 512 * 1024
MAX_EXTRACTED_BYTES = 64 * 1024
FETCH_TIMEOUT_SECONDS = 10
CHAT_SESSION_PROTOCOL = "SALIX-CHAT-SESSION/1"
CHAT_WORKER_HOST = "127.0.0.1"
CHAT_WORKER_PORT = 8766
CHAT_WORKER_TIMEOUT_SECONDS = 210
MAX_CONVERSATION_TEXT_BYTES = 128 * 1024
MAX_CONVERSATION_DELTA_EVENTS = 28


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


def _parse_conversation_probe_request(raw_body: bytes) -> int:
    if len(raw_body) > 1024:
        raise ValueError("conversation probe request is too large")

    text = raw_body.decode("utf-8", errors="strict")
    lines = [line for line in text.splitlines() if line]

    if not lines or lines[0] != CONVERSATION_PROTOCOL:
        raise ValueError("conversation protocol header is missing")

    values: dict[str, str] = {}
    allowed_keys = {
        "mode",
        "request_id",
        "text_forwarded",
        "attachments_forwarded",
        "credentials_forwarded",
        "session_forwarded",
    }

    for line in lines[1:]:
        if "=" not in line:
            raise ValueError("conversation probe metadata is malformed")

        key, value = line.split("=", 1)
        if key not in allowed_keys:
            raise ValueError("conversation probe contains an unexpected field")

        values[key] = value

    if values.get("mode") != "probe":
        raise ValueError("conversation probe mode is required")

    if values.get("text_forwarded") != "0":
        raise ValueError("conversation probe must not forward message text")

    if values.get("attachments_forwarded") != "0":
        raise ValueError("conversation probe must not forward attachments")

    if values.get("credentials_forwarded") != "0":
        raise ValueError("conversation probe must not forward credentials")

    if values.get("session_forwarded") != "0":
        raise ValueError("conversation probe must not forward session material")

    try:
        request_id = int(values.get("request_id", "0"))
    except ValueError as error:
        raise ValueError("conversation request ID is invalid") from error

    if request_id < 1 or request_id > 0xFFFFFFFF:
        raise ValueError("conversation request ID is out of range")

    return request_id


def _frame_conversation_probe(request_id: int) -> bytes:
    events = [
        ("request_started", ""),
        ("message_started", ""),
        ("text_delta", "**Remote semantic bridge online.**\n\n"),
        (
            "text_delta",
            "This response came from the modern companion through "
            "`SALIX-CONVERSATION/1` semantic events.\n\n",
        ),
        (
            "text_delta",
            "The probe did not transmit your message text, attachment paths, "
            "credentials, cookies, or session data.",
        ),
        ("message_completed", ""),
    ]

    encoded_events = [
        (event_type, event_text.encode("utf-8"))
        for event_type, event_text in events
    ]

    lines = [
        CONVERSATION_PROTOCOL,
        "status=ok",
        f"request_id={request_id}",
        f"event_count={len(encoded_events)}",
        "mode=probe",
        "text_forwarded=0",
        "attachments_forwarded=0",
        "credentials_forwarded=0",
        "session_forwarded=0",
        "transport_security=plaintext",
    ]

    for index, (event_type, event_text) in enumerate(encoded_events):
        lines.append(f"event_{index}_type={event_type}")
        lines.append(f"event_{index}_len={len(event_text)}")

    header = ("\n".join(lines) + "\n\n").encode("ascii")
    return header + b"".join(event_text for _event_type, event_text in encoded_events)


def _perform_conversation_probe(raw_body: bytes) -> bytes:
    request_id = _parse_conversation_probe_request(raw_body)
    print(
        "[conversation] probe request "
        f"id={request_id} "
        "text=0 attachments=0 credentials=0 session=0"
    )
    return _frame_conversation_probe(request_id)

def _parse_protocol_metadata(metadata: str) -> dict[str, str]:
    lines = metadata.splitlines()
    if not lines or lines[0] != CONVERSATION_PROTOCOL:
        raise ValueError("conversation protocol header is missing")

    values: dict[str, str] = {}
    for line in lines[1:]:
        if not line:
            continue
        if "=" not in line:
            raise ValueError("conversation metadata is malformed")
        key, value = line.split("=", 1)
        values[key] = value
    return values


def _parse_conversation_message_request(raw_body: bytes) -> tuple[int, str]:
    header_end = raw_body.find(b"\n\n")
    if header_end < 0:
        raise ValueError("conversation message framing is missing")

    metadata = raw_body[:header_end].decode("ascii", errors="strict")
    values = _parse_protocol_metadata(metadata)

    if values.get("mode") != "browser_relay":
        raise ValueError("browser relay mode is required")
    if values.get("text_forwarded") != "1":
        raise ValueError("browser relay requires message text")
    if values.get("attachments_forwarded") != "0":
        raise ValueError("attachments are not enabled for the first relay pass")
    if values.get("credentials_forwarded") != "0":
        raise ValueError("credentials must not cross the Salix relay")
    if values.get("session_forwarded") != "0":
        raise ValueError("browser session material must not cross the Salix relay")

    try:
        request_id = int(values.get("request_id", "0"))
        text_length = int(values.get("text_len", "-1"))
    except ValueError as error:
        raise ValueError("conversation numeric metadata is invalid") from error

    if request_id < 1 or request_id > 0xFFFFFFFF:
        raise ValueError("conversation request ID is out of range")
    if text_length < 1 or text_length > MAX_CONVERSATION_TEXT_BYTES:
        raise ValueError("conversation text length is invalid")

    text_bytes = raw_body[header_end + 2 :]
    if len(text_bytes) != text_length:
        raise ValueError("conversation text length does not match framing")

    return request_id, text_bytes.decode("utf-8", errors="strict")


def _call_chat_worker(
    request_id: int,
    text: str,
) -> str:
    payload = json.dumps(
        {
            "request_id": request_id,
            "text": text,
        },
        ensure_ascii=False,
    ).encode("utf-8")

    connection = http.client.HTTPConnection(
        CHAT_WORKER_HOST,
        CHAT_WORKER_PORT,
        timeout=CHAT_WORKER_TIMEOUT_SECONDS,
    )

    try:
        connection.request(
            "POST",
            "/v1/message",
            body=payload,
            headers={
                "Content-Type": "application/json; charset=utf-8",
                "Content-Length": str(len(payload)),
                "Connection": "close",
            },
        )
        response = connection.getresponse()
        raw = response.read(MAX_REQUEST_BYTES + 1)

        if len(raw) > MAX_REQUEST_BYTES:
            raise RuntimeError("chat worker response exceeded bridge limit")

        try:
            value = json.loads(raw.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise RuntimeError("chat worker returned invalid JSON") from error

        if response.status != 200:
            detail = value.get("error") if isinstance(value, dict) else None
            raise RuntimeError(
                str(detail or f"chat worker returned HTTP {response.status}")
            )

        if not isinstance(value, dict):
            raise RuntimeError("chat worker response must be a JSON object")
        if value.get("protocol") != CHAT_SESSION_PROTOCOL:
            raise RuntimeError("chat worker protocol mismatch")
        if value.get("status") != "ok":
            raise RuntimeError("chat worker did not complete the message")
        if value.get("request_id") != request_id:
            raise RuntimeError("chat worker request ID mismatch")

        response_text = value.get("text")
        if not isinstance(response_text, str) or not response_text:
            raise RuntimeError("chat worker returned an empty response")

        return response_text
    except (OSError, http.client.HTTPException) as error:
        raise RuntimeError(f"chat worker unavailable: {error}") from error
    finally:
        connection.close()


def _chat_worker_health() -> tuple[bool, str]:
    connection = http.client.HTTPConnection(
        CHAT_WORKER_HOST,
        CHAT_WORKER_PORT,
        timeout=2,
    )
    try:
        connection.request(
            "GET",
            "/v1/health",
            headers={"Connection": "close"},
        )
        response = connection.getresponse()
        raw = response.read(64 * 1024)

        if response.status != 200:
            return False, f"worker_http_{response.status}"

        value = json.loads(raw.decode("utf-8"))
        if not isinstance(value, dict):
            return False, "worker_invalid_response"
        if value.get("protocol") != CHAT_SESSION_PROTOCOL:
            return False, "worker_protocol_mismatch"
        if value.get("status") != "ok":
            return False, "worker_error"
        if value.get("session_ready") is not True:
            return False, "browser_login_or_thread_not_ready"

        return True, "ready"
    except Exception:
        return False, "worker_unavailable"
    finally:
        connection.close()


def _split_response_deltas(text: str) -> list[str]:
    if not text:
        return []

    chunk_size = max(
        1,
        (len(text) + MAX_CONVERSATION_DELTA_EVENTS - 1)
        // MAX_CONVERSATION_DELTA_EVENTS,
    )
    return [
        text[index : index + chunk_size]
        for index in range(0, len(text), chunk_size)
    ]


def _frame_conversation_relay(
    request_id: int,
    response_text: str,
) -> bytes:
    events: list[tuple[str, str]] = [
        ("request_started", ""),
        ("message_started", ""),
    ]
    events.extend(
        ("text_delta", delta)
        for delta in _split_response_deltas(response_text)
    )
    events.append(("message_completed", ""))

    encoded_events = [
        (event_type, event_text.encode("utf-8"))
        for event_type, event_text in events
    ]

    lines = [
        CONVERSATION_PROTOCOL,
        "status=ok",
        f"request_id={request_id}",
        f"event_count={len(encoded_events)}",
        "mode=browser_relay",
        "text_forwarded=1",
        "attachments_forwarded=0",
        "credentials_forwarded=0",
        "session_forwarded=0",
        "transport_security=trusted_lan",
    ]

    for index, (event_type, event_text) in enumerate(encoded_events):
        lines.append(f"event_{index}_type={event_type}")
        lines.append(f"event_{index}_len={len(event_text)}")

    header = ("\n".join(lines) + "\n\n").encode("ascii")
    return header + b"".join(
        event_text
        for _event_type, event_text in encoded_events
    )


def _perform_conversation_relay(raw_body: bytes) -> bytes:
    request_id, text = _parse_conversation_message_request(raw_body)

    print(
        "[conversation] browser relay request "
        f"id={request_id} "
        f"text_bytes={len(text.encode('utf-8'))} "
        "attachments=0 credentials=0 session=0"
    )

    response_text = _call_chat_worker(request_id, text)

    print(
        "[conversation] browser relay response "
        f"id={request_id} "
        f"text_bytes={len(response_text.encode('utf-8'))}"
    )

    return _frame_conversation_relay(
        request_id,
        response_text,
    )



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
    server_version = "SalixBridge/0.3"
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
            worker_ready, worker_status = _chat_worker_health()
            self._send_text(
                200,
                f"{PROTOCOL}\n"
                "status=ok\n"
                "service=salix_bridge\n"
                "probe=enabled\n"
                "conversation_probe=enabled\n"
                "conversation_relay=enabled\n"
                f"conversation_protocol={CONVERSATION_PROTOCOL}\n"
                "conversation_mode=browser_relay\n"
                "conversation_text_forwarding=enabled\n"
                "conversation_attachment_forwarding=disabled\n"
                "conversation_credential_forwarding=disabled\n"
                "conversation_session_forwarding=disabled\n"
                "conversation_transport_security=trusted_lan\n"
                f"conversation_browser_session={'ready' if worker_ready else worker_status}\n",
            )
            return

        self._send_text(
            404,
            f"{PROTOCOL}\nstatus=not_found\npath={self.path}\n",
        )

    def do_POST(self) -> None:  # noqa: N802 - BaseHTTPRequestHandler contract
        if self.path not in (
            "/v1/navigate",
            "/v1/fetch",
            "/v1/conversation/probe",
            "/v1/conversation/message",
        ):
            self._send_text(
                404,
                f"{PROTOCOL}\nstatus=not_found\npath={self.path}\n",
            )
            return

        raw_body = self._read_request_body()
        if raw_body is None:
            return

        if self.path == "/v1/conversation/probe":
            try:
                payload = _perform_conversation_probe(raw_body)
            except (UnicodeDecodeError, ValueError) as error:
                self._send_text(
                    400,
                    f"{CONVERSATION_PROTOCOL}\n"
                    "status=invalid_probe\n"
                    f"error={error}\n",
                )
                return

            self._send_bytes(
                200,
                payload,
                "application/x-salix-conversation",
            )
            return

        if self.path == "/v1/conversation/message":
            try:
                payload = _perform_conversation_relay(raw_body)
            except (UnicodeDecodeError, ValueError) as error:
                self._send_text(
                    400,
                    f"{CONVERSATION_PROTOCOL}\n"
                    "status=invalid_message\n"
                    f"error={error}\n",
                )
                return
            except RuntimeError as error:
                self._send_text(
                    503,
                    f"{CONVERSATION_PROTOCOL}\n"
                    "status=browser_relay_failed\n"
                    f"error={error}\n",
                )
                return

            self._send_bytes(
                200,
                payload,
                "application/x-salix-conversation",
            )
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
    parser.add_argument(
        "--chat-worker-host",
        default="127.0.0.1",
        help="localhost browser worker host (default: 127.0.0.1)",
    )
    parser.add_argument(
        "--chat-worker-port",
        type=int,
        default=8766,
        help="localhost browser worker port (default: 8766)",
    )
    args = parser.parse_args()

    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535")
    if args.chat_worker_host not in ("127.0.0.1", "localhost"):
        parser.error("--chat-worker-host must remain localhost-only")
    if not 1 <= args.chat_worker_port <= 65535:
        parser.error("--chat-worker-port must be between 1 and 65535")

    global CHAT_WORKER_HOST
    global CHAT_WORKER_PORT
    CHAT_WORKER_HOST = args.chat_worker_host
    CHAT_WORKER_PORT = args.chat_worker_port

    server = ThreadingHTTPServer((args.host, args.port), SalixBridgeHandler)

    print(f"Salix bridge protocol : {PROTOCOL}")
    print(f"Browser probe protocol: {PROBE_PROTOCOL}")
    print(f"Conversation protocol : {CONVERSATION_PROTOCOL}")
    print(f"Listening             : http://{args.host}:{args.port}")
    print("Modern HTTPS probe    : enabled (unauthenticated GET only)")
    print("Probe address family  : IPv4")
    print("Conversation mode     : browser-relay text baseline")
    print(
        "Chat browser worker   : "
        f"http://{CHAT_WORKER_HOST}:{CHAT_WORKER_PORT}"
    )
    print("Message forwarding    : text enabled on trusted development LAN")
    print("Sensitive forwarding  : attachments/credentials/session disabled")
    print("Browser auth/session  : remains inside the LibreWolf worker")
    print("Response path         : rendered assistant text -> semantic events")
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