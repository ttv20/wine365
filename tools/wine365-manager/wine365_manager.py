#!/usr/bin/env python3
"""Dependency-free local web interface for Wine 365 Manager."""

from __future__ import annotations

import argparse
import json
import os
import secrets
import signal
import subprocess
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import wine365_backend as backend  # noqa: E402

if HERE.name == "lib":
    INSTALL_ROOT = HERE.parent
    LAUNCHER = INSTALL_ROOT / "bin/wine365-launcher"
    ICONS = INSTALL_ROOT / "icons"
else:
    INSTALL_ROOT = HERE
    LAUNCHER = HERE / "wine365-launcher"
    ICONS = HERE / "icons"
FONT_HELPER = HERE / "register-office-cloud-fonts.sh"
UI_FILE = HERE / "ui.html"


class ManagerState:
    def __init__(self) -> None:
        self.lock = threading.RLock()
        self.config = backend.load_config()
        self.task = {"running": False, "kind": "", "status": "idle", "log": ""}
        self.cancel_event = threading.Event()
        self.process: subprocess.Popen | None = None

    def update_config(self, payload: dict) -> dict:
        with self.lock:
            for key in ("prefix", "wine", "update_url"):
                if key in payload:
                    self.config[key] = str(payload[key]).strip()
            if "desktop_copy" in payload:
                self.config["desktop_copy"] = bool(payload["desktop_copy"])
            backend.save_config(self.config)
            return dict(self.config)

    def snapshot(self) -> dict:
        with self.lock:
            config = dict(self.config)
            task = dict(self.task)
        return {
            "config": config,
            "status": backend.environment_status(config["prefix"], config["wine"]),
            "version": backend.current_version(),
            "task": task,
        }

    def output(self, line: str) -> None:
        with self.lock:
            text = self.task["log"] + line + "\n"
            self.task["log"] = text[-1_000_000:]

    def set_process(self, process: subprocess.Popen | None) -> None:
        with self.lock:
            self.process = process

    def start_task(self, kind: str, operation) -> None:
        with self.lock:
            if self.task["running"]:
                raise RuntimeError("Another operation is already running.")
            self.task = {"running": True, "kind": kind, "status": "running", "log": ""}
            self.cancel_event.clear()

        def worker() -> None:
            try:
                result = operation()
                with self.lock:
                    self.task["status"] = "completed"
                    if result:
                        self.output(str(result))
            except Exception as error:  # surfaced verbatim in the local UI
                with self.lock:
                    self.task["status"] = "failed"
                    self.output(f"ERROR: {error}")
            finally:
                with self.lock:
                    self.task["running"] = False
                    self.process = None

        threading.Thread(target=worker, name=f"wine365-{kind}", daemon=True).start()

    def cancel(self) -> None:
        self.cancel_event.set()
        with self.lock:
            process = self.process
        if process and process.poll() is None:
            try:
                os.killpg(process.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass


class ManagerServer(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, address, handler, state: ManagerState, token: str):
        super().__init__(address, handler)
        self.state = state
        self.token = token


class Handler(BaseHTTPRequestHandler):
    server: ManagerServer

    def log_message(self, format: str, *args) -> None:
        return

    def _authorized_path(self) -> tuple[str, bool]:
        parsed = urlparse(self.path)
        prefix = f"/{self.server.token}"
        authorized = parsed.path == prefix or parsed.path.startswith(prefix + "/")
        return (parsed.path[len(prefix):] or "/", authorized)

    def _json(self, data: dict, status: int = 200) -> None:
        body = json.dumps(data).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _payload(self) -> dict:
        try:
            length = int(self.headers.get("Content-Length", "0"))
            value = json.loads(self.rfile.read(length) or b"{}")
            if not isinstance(value, dict):
                raise ValueError("JSON payload must be an object.")
            return value
        except (ValueError, json.JSONDecodeError) as error:
            raise ValueError(f"Invalid request: {error}") from error

    def do_GET(self) -> None:
        path, authorized = self._authorized_path()
        if not authorized:
            self.send_error(404)
            return
        if path == "/":
            body = UI_FILE.read_text().replace("__API_BASE__", f"/{self.server.token}").encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Security-Policy", "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline'")
            self.end_headers()
            self.wfile.write(body)
        elif path == "/api/state":
            self._json(self.server.state.snapshot())
        else:
            self.send_error(404)

    def do_POST(self) -> None:
        path, authorized = self._authorized_path()
        if not authorized:
            self.send_error(404)
            return
        try:
            payload = self._payload()
            state = self.server.state
            if path == "/api/config":
                self._json({"ok": True, "config": state.update_config(payload)})
                return

            config = state.update_config(payload.get("config", {}))
            if path == "/api/environment/create":
                state.start_task("environment", lambda: backend.create_environment(
                    config["prefix"], config["wine"], False, state.output))
            elif path == "/api/environment/recreate":
                state.start_task("environment", lambda: backend.create_environment(
                    config["prefix"], config["wine"], True, state.output))
            elif path == "/api/shortcuts/create":
                files = backend.create_app_shortcuts(payload.get("apps", []), config["prefix"],
                                                     config["wine"], LAUNCHER, ICONS,
                                                     config["desktop_copy"])
                self._json({"ok": True, "message": f"Created {len(files)} shortcut file(s)."})
                return
            elif path == "/api/shortcuts/remove":
                files = backend.remove_app_shortcuts(payload.get("apps", []))
                self._json({"ok": True, "message": f"Removed {len(files)} shortcut file(s)."})
                return
            elif path == "/api/launch/app":
                pid = backend.launch_app(config["prefix"], config["wine"], str(payload.get("app", "")), FONT_HELPER)
                self._json({"ok": True, "message": f"Application started (PID {pid})."})
                return
            elif path == "/api/launch/tool":
                pid = backend.launch_tool(config["prefix"], config["wine"], str(payload.get("tool", "")))
                message = "Wine processes stopped." if pid is None else f"Tool started (PID {pid})."
                self._json({"ok": True, "message": message})
                return
            elif path == "/api/update/start":
                def update() -> str:
                    try:
                        backend.stop_wine(config["prefix"], config["wine"])
                        state.output("Stopped the selected Wine environment before updating.")
                    except (FileNotFoundError, OSError):
                        pass
                    return backend.update_wine365(config["update_url"], state.output,
                                                  state.cancel_event, state.set_process)
                state.start_task("update", update)
            elif path == "/api/remove/start":
                state.start_task("remove", lambda: backend.remove_wine365(
                    config["prefix"], bool(payload.get("remove_prefix", False)), state.output))
            elif path == "/api/task/cancel":
                state.cancel()
                self._json({"ok": True, "message": "Cancellation requested."})
                return
            elif path == "/api/shutdown":
                self._json({"ok": True, "message": "Wine 365 Manager stopped."})
                threading.Thread(target=self.server.shutdown, daemon=True).start()
                return
            else:
                self.send_error(404)
                return
            self._json({"ok": True, "message": "Operation started."})
        except Exception as error:
            self._json({"ok": False, "error": str(error)}, 400)


def main() -> int:
    parser = argparse.ArgumentParser(description="Wine 365 Manager")
    parser.add_argument("--no-browser", action="store_true", help="print the URL instead of opening it")
    parser.add_argument("--install-shortcut", action="store_true", help=argparse.SUPPRESS)
    args = parser.parse_args()

    if args.install_shortcut:
        path = backend.install_manager_shortcut(LAUNCHER.with_name("wine365-manager"), ICONS)
        print(path)
        return 0
    if not UI_FILE.is_file():
        parser.error(f"UI file is missing: {UI_FILE}")

    state = ManagerState()
    token = secrets.token_urlsafe(24)
    server = ManagerServer(("127.0.0.1", 0), Handler, state, token)
    url = f"http://127.0.0.1:{server.server_port}/{token}/"
    print(f"Wine 365 Manager: {url}", flush=True)
    if not args.no_browser:
        threading.Timer(0.2, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever(poll_interval=0.25)
    except KeyboardInterrupt:
        pass
    finally:
        state.cancel()
        server.server_close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
