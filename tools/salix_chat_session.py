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
import configparser
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


def _fallback_profile_directory() -> Path:
    local_app_data = os.environ.get("LOCALAPPDATA")
    if local_app_data:
        return Path(local_app_data) / "SalixWeb32" / "LibreWolfProfile"
    return Path.home() / "AppData" / "Local" / "SalixWeb32" / "LibreWolfProfile"


def _candidate_profile_roots() -> list[Path]:
    roots: list[Path] = []

    for variable in ("LOCALAPPDATA", "APPDATA"):
        value = os.environ.get(variable)
        if not value:
            continue

        base = Path(value)
        roots.extend(
            [
                base / "librewolf",
                base / "LibreWolf",
            ]
        )

    unique: list[Path] = []
    seen: set[str] = set()

    for root in roots:
        key = os.path.normcase(str(root))
        if key in seen:
            continue
        seen.add(key)
        unique.append(root)

    return unique


def _read_ini(path: Path) -> configparser.RawConfigParser | None:
    parser = configparser.RawConfigParser()

    try:
        with path.open("r", encoding="utf-8") as handle:
            parser.read_file(handle)
    except (OSError, UnicodeError, configparser.Error):
        return None

    return parser


def _resolve_profile_path(root: Path, value: str) -> Path:
    candidate = Path(value)

    if not candidate.is_absolute():
        candidate = root / candidate

    return candidate.resolve()


def _profile_matches_browser_install(
    profile_directory: Path,
    browser_path: Path,
) -> bool:
    compatibility_path = profile_directory / "compatibility.ini"
    parser = _read_ini(compatibility_path)

    if parser is None or not parser.has_section("Compatibility"):
        return False

    expected = os.path.normcase(
        os.path.normpath(str(browser_path.parent.resolve()))
    )

    for key in ("LastPlatformDir", "LastAppDir"):
        value = parser.get("Compatibility", key, fallback="").strip()
        if not value:
            continue

        actual = os.path.normcase(os.path.normpath(value))
        if actual == expected:
            return True

    return False


def _discover_install_default_profiles(
    browser_path: Path,
) -> list[tuple[Path, str]]:
    discovered: list[tuple[Path, str]] = []
    seen: set[str] = set()

    for root in _candidate_profile_roots():
        for ini_name in ("profiles.ini", "installs.ini"):
            ini_path = root / ini_name
            if not ini_path.is_file():
                continue

            parser = _read_ini(ini_path)
            if parser is None:
                continue

            for section in parser.sections():
                if ini_name == "profiles.ini":
                    if not section.lower().startswith("install"):
                        continue

                path_value = parser.get(
                    section,
                    "Default",
                    fallback="",
                ).strip()

                if not path_value:
                    continue

                try:
                    profile_path = _resolve_profile_path(
                        root,
                        path_value,
                    )
                except OSError:
                    continue

                if not profile_path.is_dir():
                    continue

                key = os.path.normcase(str(profile_path))
                if key in seen:
                    continue

                seen.add(key)
                source = (
                    f"{ini_name} [{section}] install default"
                )
                discovered.append((profile_path, source))

    if len(discovered) <= 1:
        return discovered

    matching = [
        item
        for item in discovered
        if _profile_matches_browser_install(
            item[0],
            browser_path,
        )
    ]

    if len(matching) == 1:
        return matching

    return discovered


def _discover_installed_profiles() -> list[tuple[Path, str, bool]]:
    discovered: list[tuple[Path, str, bool]] = []
    seen: set[str] = set()
    roots = _candidate_profile_roots()

    for root in roots:
        ini_path = root / "profiles.ini"
        if not ini_path.is_file():
            continue

        parser = configparser.RawConfigParser()

        try:
            parser.read(ini_path, encoding="utf-8")
        except (OSError, configparser.Error):
            continue

        for section in parser.sections():
            if not section.lower().startswith("profile"):
                continue

            path_value = parser.get(section, "Path", fallback="").strip()
            if not path_value:
                continue

            is_relative = parser.getboolean(
                section,
                "IsRelative",
                fallback=True,
            )
            name = parser.get(section, "Name", fallback=section).strip()
            is_default = parser.getboolean(
                section,
                "Default",
                fallback=False,
            )

            candidates: list[Path] = []

            if is_relative:
                candidates.append(root / Path(path_value))
                for alternate_root in roots:
                    candidates.append(alternate_root / Path(path_value))
            else:
                candidates.append(Path(path_value))

            for candidate in candidates:
                if not candidate.is_dir():
                    continue

                resolved = candidate.resolve()
                key = os.path.normcase(str(resolved))
                if key in seen:
                    continue

                seen.add(key)
                discovered.append((resolved, name, is_default))
                break

    # Some LibreWolf installations keep the profile directories in LocalAppData
    # even when profiles.ini is absent or stored elsewhere. Preserve those as
    # visible candidates instead of silently creating a second browser identity.
    for root in roots:
        profiles_root = root / "Profiles"
        if not profiles_root.is_dir():
            continue

        try:
            children = list(profiles_root.iterdir())
        except OSError:
            continue

        for child in children:
            if not child.is_dir():
                continue

            resolved = child.resolve()
            key = os.path.normcase(str(resolved))
            if key in seen:
                continue

            seen.add(key)
            discovered.append((resolved, child.name, False))

    return discovered


def _select_profile(
    explicit: str | None,
    browser_path: Path,
) -> tuple[Path, str]:
    requested = explicit or os.environ.get("SALIX_LIBREWOLF_PROFILE")

    if requested:
        path = Path(requested).expanduser().resolve()
        if not path.is_dir():
            raise RuntimeError(
                f"requested LibreWolf profile does not exist: {path}"
            )
        return path, "explicit LibreWolf profile"

    install_defaults = _discover_install_default_profiles(
        browser_path,
    )

    if len(install_defaults) == 1:
        return (
            install_defaults[0][0],
            "installed LibreWolf per-install default profile",
        )

    if len(install_defaults) > 1:
        choices = "\n".join(
            f"  {path}  ({source})"
            for path, source in install_defaults
        )
        raise RuntimeError(
            "Multiple LibreWolf per-install default profiles were discovered "
            "and the active installation could not be selected safely. "
            "Re-run with --profile and one of:\n"
            + choices
        )

    profiles = _discover_installed_profiles()

    defaults = [item for item in profiles if item[2]]
    if len(defaults) == 1:
        return (
            defaults[0][0],
            "installed LibreWolf legacy default profile",
        )

    if len(profiles) == 1:
        return profiles[0][0], "installed LibreWolf profile"

    # Final metadata-free fallback only. Prefer the generated per-install-style
    # profile name when it uniquely identifies one candidate.
    generated_defaults = [
        item
        for item in profiles
        if item[0].name.lower().endswith(".default-default")
    ]
    if not defaults and len(generated_defaults) == 1:
        return (
            generated_defaults[0][0],
            "installed LibreWolf default-default profile",
        )

    if profiles:
        choices = "\n".join(
            f"  {path}  ({name}{', legacy default' if is_default else ''})"
            for path, name, is_default in profiles
        )
        raise RuntimeError(
            "Multiple LibreWolf profiles were discovered and no single profile "
            "could be selected safely. Re-run with --profile and one of:\n"
            + choices
        )

    fallback = _fallback_profile_directory().resolve()
    return fallback, "Salix fallback profile (no installed profile discovered)"


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
        # explicit -profile argument gives this worker the selected persistent
        # LibreWolf profile, normally the user's installed default profile.
        # Do not open the same profile simultaneously in another LibreWolf process.
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
        default=None,
        help=(
            "LibreWolf profile directory; when omitted the installed LibreWolf "
            "default profile is discovered automatically"
        ),
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
            "open the selected LibreWolf profile normally without WebDriver "
            "for manual authentication if needed"
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

    try:
        profile_directory, profile_source = _select_profile(
            args.profile,
            browser_path,
        )
    except Exception as error:
        print(
            f"ERROR: {type(error).__name__}: {error}",
            file=sys.stderr,
        )
        return 1

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
    print(f"Profile source        : {profile_source}")
    print(f"Profile path          : {profile_directory}")
    print(f"Worker endpoint       : http://{args.listen_host}:{args.listen_port}")
    print("Authentication        : reused from selected LibreWolf profile")
    print("Active conversation   : whichever ChatGPT thread is open")
    print("Forwarding            : message text + rendered assistant text only")
    print("Cookies/credentials   : never exposed by this worker API")
    print()

    try:
        print(
            "Close any ordinary LibreWolf window using this profile "
            "before continuing."
        )
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
    print(
        "The selected installed profile should already contain your "
        "normal ChatGPT login/session."
    )
    print("Open the desired ChatGPT thread if it is not already open.")
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
