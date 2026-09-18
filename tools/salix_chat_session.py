#!/usr/bin/env python3
"""Own a visible LibreWolf session and expose a tiny localhost chat relay.

This process is intentionally separate from salix_bridge.py:

- LibreWolf owns the real chatgpt.com login/session.
- The user performs authentication directly in the visible browser.
- SalixWeb32 never receives browser cookies, credentials, or session storage.
- Only user message text and rendered assistant response text cross the relay.

The first baseline operates on the ChatGPT conversation that is currently open in the
owned LibreWolf window.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

SESSION_PROTOCOL = "SALIX-CHAT-SESSION/1"
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8766
DEFAULT_URL = "https://chatgpt.com/"
MAX_MESSAGE_BYTES = 128 * 1024
DEFAULT_RESPONSE_TIMEOUT_SECONDS = 180.0
POLL_SECONDS = 0.20
STABLE_SECONDS = 2.0


def _default_profile_directory() -> Path:
    local_app_data = os.environ.get("LOCALAPPDATA")
    if local_app_data:
        return Path(local_app_data) / "SalixWeb32" / "LibreWolfProfile"
    return Path.home() / "AppData" / "Local" / "SalixWeb32" / "LibreWolfProfile"


def _candidate_librewolf_paths() -> list[Path]:
    candidates: list[Path] = []
    explicit = os.environ.get("SALIX_LIBREWOLF_BINARY")
    if explicit:
        candidates.append(Path(explicit))

    for variable in ("ProgramFiles", "ProgramFiles(x86)", "LOCALAPPDATA"):
        root = os.environ.get(variable)
        if not root:
            continue

        root_path = Path(root)
        if variable == "LOCALAPPDATA":
            candidates.extend(
                [
                    root_path / "Programs" / "LibreWolf" / "librewolf.exe",
                    root_path / "LibreWolf" / "librewolf.exe",
                ]
            )
        else:
            candidates.append(root_path / "LibreWolf" / "librewolf.exe")

    return candidates


def _find_librewolf(explicit: str | None) -> Path:
    candidates = [Path(explicit)] if explicit else _candidate_librewolf_paths()

    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()

    searched = "\n".join(f"  {candidate}" for candidate in candidates)
    raise RuntimeError(
        "LibreWolf executable was not found. "
        "Pass --browser C:\\path\\to\\librewolf.exe or set "
        "SALIX_LIBREWOLF_BINARY.\nSearched:\n"
        + (searched or "  no candidate paths")
    )


def _json_bytes(value: dict[str, Any]) -> bytes:
    return json.dumps(value, ensure_ascii=False).encode("utf-8")


def _prepare_login_profile(
    browser_path: Path,
    profile_directory: Path,
    start_url: str,
) -> int:
    """Open the dedicated profile in ordinary LibreWolf without WebDriver.

    Google may reject OAuth login from an automation-controlled browser. This
    bootstrap mode lets the user authenticate normally once, then reuse the
    resulting browser profile in the relay worker.
    """

    profile_directory.mkdir(parents=True, exist_ok=True)

    command = [
        str(browser_path),
        "-no-remote",
        "-profile",
        str(profile_directory),
        start_url,
    ]

    print("Opening ordinary LibreWolf for manual authentication.")
    print(f"LibreWolf binary   : {browser_path}")
    print(f"Persistent profile : {profile_directory}")
    print(f"Start URL          : {start_url}")
    print()
    print("Log in to ChatGPT normally in this browser window.")
    print("When ChatGPT is usable, CLOSE the LibreWolf window.")
    print("The command will then return and the profile will be ready for the worker.")
    print()

    try:
        process = subprocess.Popen(command)
        return int(process.wait())
    except OSError as error:
        raise RuntimeError(
            f"failed to launch ordinary LibreWolf: {error}"
        ) from error


class ChatBrowserSession:
    def __init__(
        self,
        browser_path: Path,
        profile_directory: Path,
        geckodriver_path: str | None,
        start_url: str,
        response_timeout_seconds: float,
    ) -> None:
        self.browser_path = browser_path
        self.profile_directory = profile_directory
        self.geckodriver_path = geckodriver_path
        self.start_url = start_url
        self.response_timeout_seconds = response_timeout_seconds

        self.driver = None
        self.lock = threading.Lock()
        self.last_error = ""

    def start(self) -> None:
        try:
            from selenium import webdriver
            from selenium.webdriver.firefox.options import Options
            from selenium.webdriver.firefox.service import Service
        except ImportError as error:
            raise RuntimeError(
                "Selenium is not installed. Run tools\\setup_chat_session.bat "
                "on the modern companion machine."
            ) from error

        self.profile_directory.mkdir(parents=True, exist_ok=True)

        options = Options()
        options.binary_location = str(self.browser_path)

        # Selenium/geckodriver normally creates a temporary Firefox profile. The
        # explicit -profile argument gives this worker a dedicated persistent
        # profile so a user-authenticated ChatGPT session can survive restarts.
        # Do not point this at a profile simultaneously opened by another browser.
        options.add_argument("-profile")
        options.add_argument(str(self.profile_directory))

        service = (
            Service(executable_path=self.geckodriver_path)
            if self.geckodriver_path
            else Service()
        )

        self.driver = webdriver.Firefox(
            service=service,
            options=options,
        )
        self.driver.get(self.start_url)

    def stop(self) -> None:
        driver = self.driver
        self.driver = None
        if driver is not None:
            try:
                driver.quit()
            except Exception:
                pass

    def _require_driver(self):
        if self.driver is None:
            raise RuntimeError("LibreWolf session is not running")
        return self.driver

    @staticmethod
    def _composer_selectors() -> tuple[str, ...]:
        return (
            'textarea[name="prompt-textarea"]',
            'textarea[data-testid="prompt-textarea"]',
            '#prompt-textarea',
            '[contenteditable="true"][data-testid="prompt-textarea"]',
            'div[contenteditable="true"]',
        )

    def _find_composer(self):
        from selenium.webdriver.common.by import By

        driver = self._require_driver()
        for selector in self._composer_selectors():
            elements = driver.find_elements(By.CSS_SELECTOR, selector)
            for element in elements:
                try:
                    if element.is_displayed() and element.is_enabled():
                        return element
                except Exception:
                    continue
        return None

    def _assistant_snapshots(self) -> list[str]:
        driver = self._require_driver()

        script = r"""
const cleanText = (node) => {
    if (!node) return "";
    const clone = node.cloneNode(true);
    clone.querySelectorAll(
        "button, svg, [aria-hidden='true'], [data-testid='copy-turn-action-button']"
    ).forEach((child) => child.remove());
    return (clone.innerText || clone.textContent || "").trim();
};

const roleNodes = Array.from(
    document.querySelectorAll("[data-message-author-role='assistant']")
).filter((node) => {
    const rect = node.getBoundingClientRect();
    return rect.width > 0 && rect.height > 0;
});

if (roleNodes.length) {
    return roleNodes.map((node) => {
        const markdown = node.querySelector(".markdown");
        return cleanText(markdown || node);
    }).filter(Boolean);
}

const turns = Array.from(
    document.querySelectorAll("article[data-testid^='conversation-turn-']")
);

return turns.map((turn) => {
    const markdown = turn.querySelector(".markdown");
    if (!markdown) return "";
    return cleanText(markdown);
}).filter(Boolean);
"""
        value = driver.execute_script(script)
        if not isinstance(value, list):
            return []
        return [str(item).strip() for item in value if str(item).strip()]

    def _generation_active(self) -> bool:
        driver = self._require_driver()
        script = r"""
const selectors = [
    "button[data-testid='stop-button']",
    "button[aria-label='Stop generating']",
    "button[aria-label='Stop streaming']"
];
return selectors.some((selector) =>
    Array.from(document.querySelectorAll(selector)).some((node) => {
        const rect = node.getBoundingClientRect();
        return rect.width > 0 && rect.height > 0 && !node.disabled;
    })
);
"""
        return bool(driver.execute_script(script))

    def get_status(self) -> dict[str, Any]:
        with self.lock:
            try:
                driver = self._require_driver()
                composer_ready = self._find_composer() is not None
                current_url = str(driver.current_url or "")

                session_status = "ready" if composer_ready else "login_or_thread_not_ready"
                lowered_url = current_url.lower()

                if (
                    "accounts.google.com" in lowered_url and
                    (
                        "/signin/rejected" in lowered_url or
                        "rejected?" in lowered_url
                    )
                ):
                    session_status = "google_oauth_rejected_use_prepare_login"

                return {
                    "protocol": SESSION_PROTOCOL,
                    "status": "ok",
                    "browser": "LibreWolf",
                    "browser_binary": str(self.browser_path),
                    "profile": str(self.profile_directory),
                    "current_url": current_url,
                    "title": str(driver.title or ""),
                    "composer_ready": composer_ready,
                    "session_ready": composer_ready,
                    "session_status": session_status,
                    "last_error": self.last_error,
                }
            except Exception as error:
                self.last_error = f"{type(error).__name__}: {error}"
                return {
                    "protocol": SESSION_PROTOCOL,
                    "status": "error",
                    "browser": "LibreWolf",
                    "composer_ready": False,
                    "session_ready": False,
                    "session_status": "worker_error",
                    "last_error": self.last_error,
                }

    def _enter_message(self, composer, text: str) -> None:
        from selenium.webdriver.common.keys import Keys

        composer.click()

        # Clear any unsent draft in the dedicated automation profile.
        composer.send_keys(Keys.CONTROL, "a")
        composer.send_keys(Keys.BACKSPACE)

        lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
        for index, line in enumerate(lines):
            if line:
                composer.send_keys(line)
            if index + 1 < len(lines):
                composer.send_keys(Keys.SHIFT, Keys.ENTER)

        # Prefer the page's own enabled Send control. Fall back to Enter so the
        # worker remains usable across small ChatGPT markup changes.
        from selenium.webdriver.common.by import By

        for selector in (
            "button[data-testid='send-button']",
            "button[aria-label='Send prompt']",
            "button[aria-label='Send message']",
        ):
            buttons = self._require_driver().find_elements(
                By.CSS_SELECTOR,
                selector,
            )
            for button in buttons:
                try:
                    if button.is_displayed() and button.is_enabled():
                        button.click()
                        return
                except Exception:
                    continue

        composer.send_keys(Keys.ENTER)

    def send_message(self, text: str) -> str:
        if not text:
            raise ValueError("message text is empty")

        encoded = text.encode("utf-8")
        if len(encoded) > MAX_MESSAGE_BYTES:
            raise ValueError("message text exceeds relay limit")

        with self.lock:
            driver = self._require_driver()
            composer = self._find_composer()
            if composer is None:
                raise RuntimeError(
                    "ChatGPT composer is not available. "
                    "Log in and open a conversation in the LibreWolf window."
                )

            before = self._assistant_snapshots()
            before_count = len(before)
            before_last = before[-1] if before else ""

            self.last_error = ""
            self._enter_message(composer, text)

            deadline = time.monotonic() + self.response_timeout_seconds
            response_text = ""
            last_change = time.monotonic()
            observed_response = False

            while time.monotonic() < deadline:
                snapshots = self._assistant_snapshots()

                candidate = ""
                if len(snapshots) > before_count:
                    candidate = snapshots[-1]
                elif snapshots:
                    last = snapshots[-1]
                    if last != before_last:
                        candidate = last

                if candidate:
                    observed_response = True
                    if candidate != response_text:
                        response_text = candidate
                        last_change = time.monotonic()

                active = self._generation_active()

                if (
                    observed_response
                    and response_text
                    and not active
                    and time.monotonic() - last_change >= STABLE_SECONDS
                ):
                    return response_text

                time.sleep(POLL_SECONDS)

            if response_text:
                return response_text

            raise TimeoutError(
                "Timed out waiting for a rendered assistant response. "
                "The browser remains open for inspection."
            )


class ChatSessionHandler(BaseHTTPRequestHandler):
    server_version = "SalixChatSession/0.1"
    protocol_version = "HTTP/1.0"

    @property
    def session(self) -> ChatBrowserSession:
        return self.server.session  # type: ignore[attr-defined]

    def _send_json(self, status: int, value: dict[str, Any]) -> None:
        payload = _json_bytes(value)
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(payload)

    def _read_json(self) -> dict[str, Any] | None:
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

        if content_length < 1 or content_length > MAX_MESSAGE_BYTES * 2:
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

    def do_GET(self) -> None:  # noqa: N802
        if self.path != "/v1/health":
            self._send_json(
                404,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "not_found",
                },
            )
            return

        self._send_json(200, self.session.get_status())

    def do_POST(self) -> None:  # noqa: N802
        if self.path != "/v1/message":
            self._send_json(
                404,
                {
                    "protocol": SESSION_PROTOCOL,
                    "status": "not_found",
                },
            )
            return

        request = self._read_json()
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
            response_text = self.session.send_message(text)
        except Exception as error:
            detail = f"{type(error).__name__}: {error}"
            self.session.last_error = detail
            print(f"[chat-session] request id={request_id} failed: {detail}")
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

    def log_message(self, format: str, *args: object) -> None:
        print(f"[chat-session:{self.client_address[0]}] {format % args}")


class ChatSessionServer(ThreadingHTTPServer):
    def __init__(
        self,
        server_address,
        handler_class,
        session: ChatBrowserSession,
    ) -> None:
        super().__init__(server_address, handler_class)
        self.session = session


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Own a visible LibreWolf ChatGPT session for the SalixWeb32 "
            "modern-side conversation relay."
        )
    )
    parser.add_argument(
        "--browser",
        default=None,
        help=(
            "path to librewolf.exe; otherwise common install locations and "
            "SALIX_LIBREWOLF_BINARY are checked"
        ),
    )
    parser.add_argument(
        "--profile",
        default=str(_default_profile_directory()),
        help="dedicated persistent LibreWolf profile directory",
    )
    parser.add_argument(
        "--geckodriver",
        default=os.environ.get("SE_GECKODRIVER"),
        help=(
            "optional geckodriver.exe path; when omitted Selenium Manager "
            "may resolve/download it"
        ),
    )
    parser.add_argument(
        "--url",
        default=DEFAULT_URL,
        help="initial page opened in LibreWolf",
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
    parser.add_argument(
        "--prepare-login",
        action="store_true",
        help=(
            "open the dedicated profile in ordinary LibreWolf without "
            "WebDriver so manual ChatGPT/Google authentication can be completed"
        ),
    )
    args = parser.parse_args()

    if args.listen_host not in ("127.0.0.1", "localhost"):
        parser.error(
            "the browser worker is intentionally localhost-only; "
            "use 127.0.0.1 or localhost"
        )

    if not 1 <= args.listen_port <= 65535:
        parser.error("--listen-port must be between 1 and 65535")

    browser_path = _find_librewolf(args.browser)
    profile_directory = Path(args.profile).expanduser().resolve()

    if args.prepare_login:
        try:
            exit_code = _prepare_login_profile(
                browser_path=browser_path,
                profile_directory=profile_directory,
                start_url=args.url,
            )
        except Exception as error:
            print(
                f"ERROR: {type(error).__name__}: {error}",
                file=sys.stderr,
            )
            return 1

        if exit_code != 0:
            print(
                f"LibreWolf exited with code {exit_code}.",
                file=sys.stderr,
            )
            return exit_code

        print()
        print("Manual-login profile preparation finished.")
        print("Now start the relay worker with:")
        print("  python tools\\salix_chat_session.py")
        return 0

    session = ChatBrowserSession(
        browser_path=browser_path,
        profile_directory=profile_directory,
        geckodriver_path=args.geckodriver,
        start_url=args.url,
        response_timeout_seconds=args.response_timeout,
    )

    print(f"Chat session protocol : {SESSION_PROTOCOL}")
    print(f"LibreWolf binary      : {browser_path}")
    print(f"Persistent profile    : {profile_directory}")
    print(f"Worker endpoint       : http://{args.listen_host}:{args.listen_port}")
    print("Authentication        : manual, inside visible LibreWolf")
    print("Active conversation   : whichever ChatGPT thread is open")
    print("Forwarding            : message text + rendered assistant text only")
    print("Cookies/credentials   : never exposed by this worker API")
    print()

    try:
        print("Starting LibreWolf...")
        session.start()
    except Exception as error:
        print(f"ERROR: {type(error).__name__}: {error}", file=sys.stderr)
        return 1

    server = ChatSessionServer(
        (args.listen_host, args.listen_port),
        ChatSessionHandler,
        session,
    )

    print("LibreWolf started.")
    print("Log in to ChatGPT in the visible window and open the desired thread.")
    print("When the composer is visible, salix_bridge.py will report relay ready.")
    print("Press Ctrl+C here to stop the browser worker.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping Salix chat session.")
    finally:
        server.server_close()
        session.stop()

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
