#!/usr/bin/env python3
"""S01 desktop smoke test for the frameless Qt Widgets baseline.

The test intentionally uses a staging database prepared by
NiumaV3StagingPathTest.  It never selects a user work directory or reads the
production Data folder.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--data-dir", type=Path, required=True)
    parser.add_argument("--runtime-bin", type=Path, required=True)
    parser.add_argument("--evidence-dir", type=Path, required=True)
    return parser.parse_args()


def wait_for_window(process_id: int):
    from pywinauto import Application

    deadline = time.monotonic() + 20
    last_error: Exception | None = None
    while time.monotonic() < deadline:
        try:
            app = Application(backend="win32").connect(process=process_id, timeout=2)
            visible_windows = [candidate for candidate in app.windows() if candidate.is_visible()]
            if not visible_windows:
                raise RuntimeError("no visible top-level window")
            window = max(
                visible_windows,
                key=lambda candidate: candidate.rectangle().width() * candidate.rectangle().height(),
            )
            return window
        except Exception as error:  # Window construction is asynchronous.
            last_error = error
            time.sleep(0.25)
    raise RuntimeError(f"Qt main window was not discoverable: {last_error}")


def visible_window_count(process_id: int) -> int:
    from pywinauto import Application

    app = Application(backend="win32").connect(process=process_id, timeout=2)
    return sum(candidate.is_visible() for candidate in app.windows())


def visible_dialog(process_id: int, main_window):
    from pywinauto import Application

    app = Application(backend="win32").connect(process=process_id, timeout=2)
    dialogs = [candidate for candidate in app.windows()
               if candidate.is_visible() and candidate.handle != main_window.handle]
    if not dialogs:
        raise RuntimeError("task dialog was not visible")
    return min(dialogs, key=lambda candidate: candidate.rectangle().width()
               * candidate.rectangle().height())


def save_screenshot(window, destination: Path) -> str:
    try:
        window.capture_as_image().save(destination)
        return str(destination)
    except Exception as error:  # Evidence capture must not hide UI failures.
        error_path = destination.with_suffix(".txt")
        error_path.write_text(str(error), encoding="utf-8")
        return f"unavailable: {error_path.name}: {error}"


def click_navigation(window, index: int, height: int) -> None:
    # The production UI uses a 70px title bar and 48px navigation rows.
    # Seven normal rows are contiguous; Settings is pinned above the 36px bar.
    if index < 7:
        y = 70 + 24 + index * 48
    else:
        y = height - 36 - 28
    x = 110
    lparam = (y << 16) | x
    window.send_message(0x0201, 0x0001, lparam)  # WM_LBUTTONDOWN / MK_LBUTTON
    window.send_message(0x0202, 0x0000, lparam)  # WM_LBUTTONUP


def send_key(window, virtual_key: int) -> None:
    window.send_message(0x0100, virtual_key, 0)  # WM_KEYDOWN
    window.send_message(0x0101, virtual_key, 0)  # WM_KEYUP


def send_ctrl_n(window) -> None:
    window.send_message(0x0100, 0x11, 0)  # VK_CONTROL / WM_KEYDOWN
    send_key(window, 0x4E)  # N
    window.send_message(0x0101, 0x11, 0)  # VK_CONTROL / WM_KEYUP


def main() -> int:
    args = parse_args()
    if not args.exe.is_file():
        raise FileNotFoundError(args.exe)
    if not (args.data_dir / "database.db").is_file():
        raise FileNotFoundError("staging database.db is missing; run CTest first")

    args.evidence_dir.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment["NIUMA_DATA_DIR"] = str(args.data_dir)
    environment["PATH"] = f"{args.runtime_bin};{environment.get('PATH', '')}"
    process = subprocess.Popen([str(args.exe)], env=environment)
    evidence = {"processId": process.pid, "navigationRows": [], "screenshots": []}

    try:
        window = wait_for_window(process.pid)
        rectangle = window.rectangle()
        if rectangle.width() != 1440 or rectangle.height() != 900:
            raise RuntimeError(f"unexpected main-window size: {rectangle}")
        evidence["windowRectangle"] = [rectangle.left, rectangle.top, rectangle.right, rectangle.bottom]
        evidence["windowText"] = window.window_text()
        evidence["screenshots"].append(save_screenshot(
            window, args.evidence_dir / "e01-main-window.png"))

        window.move_window(rectangle.left, rectangle.top, 1280, 720, repaint=True)
        time.sleep(0.2)
        medium_rectangle = window.rectangle()
        if medium_rectangle.width() != 1280 or medium_rectangle.height() != 720:
            raise RuntimeError(f"unexpected medium-window size: {medium_rectangle}")
        window.move_window(rectangle.left, rectangle.top, 940, 580, repaint=True)
        time.sleep(0.2)
        small_rectangle = window.rectangle()
        if small_rectangle.width() != 940 or small_rectangle.height() != 580:
            raise RuntimeError(f"unexpected small-window size: {small_rectangle}")
        window.move_window(rectangle.left, rectangle.top, 1440, 900, repaint=True)
        time.sleep(0.2)
        rectangle = window.rectangle()
        evidence["testedSizes"] = [[medium_rectangle.width(), medium_rectangle.height()],
                                    [small_rectangle.width(), small_rectangle.height()],
                                    [rectangle.width(), rectangle.height()]]

        for index in range(8):
            click_navigation(window, index, rectangle.height())
            time.sleep(0.2)
            if process.poll() is not None:
                raise RuntimeError(f"application exited while navigating row {index}")
            evidence["navigationRows"].append(index)
        evidence["screenshots"].append(save_screenshot(
            window, args.evidence_dir / "e02-navigation.png"))

        send_ctrl_n(window)
        time.sleep(0.2)
        if visible_window_count(process.pid) < 2:
            raise RuntimeError("Ctrl+N did not open a visible task dialog")
        task_dialog = visible_dialog(process.pid, window)
        task_dialog.close()
        time.sleep(0.2)
        if task_dialog.is_visible():
            raise RuntimeError("task dialog did not close")
        send_key(window, 0x74)  # VK_F5
        time.sleep(0.2)
        if process.poll() is not None:
            raise RuntimeError("application exited after keyboard shortcuts")
        evidence["shortcuts"] = ["Ctrl+N", "F5"]
        evidence["dialogClose"] = "WM_CLOSE"
        evidence["desktopEscInjection"] = "blocked: no active desktop for pywinauto focus/type_keys; Qt contract covers Esc"

        window.minimize()
        time.sleep(0.2)
        window.restore()
        window.maximize()
        time.sleep(0.2)
        window.restore()
        window.close()
        process.wait(timeout=10)
        if process.returncode != 0:
            raise RuntimeError(f"application exit code: {process.returncode}")
        evidence["windowOperations"] = ["minimize", "restore", "maximize", "restore", "close"]
        evidence["exitCode"] = process.returncode
        (args.evidence_dir / "qt-desktop-e2e-session.json").write_text(
            json.dumps(evidence, ensure_ascii=False, indent=2), encoding="utf-8")
        return 0
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=5)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"S01 Qt desktop E2E failed: {error}", file=sys.stderr)
        raise SystemExit(1)
