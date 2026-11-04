#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Christoph Krogmann
# SPDX-License-Identifier: GPL-3.0-only
"""Erzeugt das Social-Preview-Bild für GitHub (docs/social-preview.png).

    python3 tools/make-social-preview.py

GitHub zeigt dieses Bild als Vorschaukarte, wenn jemand einen Link auf das
Projekt bei Mastodon, Reddit, Discord oder ähnlichem teilt. Ohne eigenes Bild
erzeugt GitHub eine schmucklose Standardkarte.

Die Datei muss anschließend von Hand hochgeladen werden:
    Settings → General → Social preview → Upload an image

Vorgabe von GitHub: 1280×640 px, höchstens 1 MB. Weil Vorschaukarten in
Zeitleisten klein dargestellt werden, ist der Text bewusst groß und knapp —
alles unter ~28 px wäre dort nicht mehr lesbar.

Gebraucht wird docs/screenshot.png. Das Bildschirmfoto wird von Hand erstellt
und dort abgelegt; dieselbe Datei verwendet auch die AppStream-Beschreibung.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

# Muss VOR dem Qt-Import stehen: Textausgabe braucht die Schriftdatenbank und
# damit eine QGuiApplication, aber kein Display.
os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from PyQt6.QtCore import QRectF, Qt                                   # noqa: E402
from PyQt6.QtGui import (                                             # noqa: E402
    QBrush, QColor, QFont, QGuiApplication, QImage, QLinearGradient,
    QPainter, QPen,
)

REPO = Path(__file__).resolve().parent.parent
ICON = REPO / "data/icons/hicolor/scalable/apps/io.github.neklor.Inkpot.svg"
SHOT = REPO / "docs/screenshot.png"
OUT = REPO / "docs/social-preview.png"

W, H = 1280, 640
BG_TOP = QColor("#38405A")
BG_BOTTOM = QColor("#222736")
TITLE = QColor("#FFFFFF")
BODY = QColor("#C3CBDC")
ACCENT = QColor("#7D99FF")
PANEL = QColor("#1A1E2A")
PANEL_EDGE = QColor("#4E5872")


def _font(size: int, bold: bool = False) -> QFont:
    f = QFont()
    f.setPointSize(size)
    f.setBold(bold)
    return f


def _icon(size: int) -> QImage:
    """Rendert das Programmsymbol aus dem SVG.

    PyQt6 bringt hier kein QtSvg mit, deshalb übernimmt das GdkPixbuf (auf
    Mint und Ubuntu vorhanden) oder ersatzweise rsvg-convert."""
    if not ICON.exists():
        raise SystemExit(f"FEHLER: {ICON} fehlt.")

    try:
        import gi
        gi.require_version("GdkPixbuf", "2.0")
        from gi.repository import GdkPixbuf

        pixbuf = GdkPixbuf.Pixbuf.new_from_file_at_size(str(ICON), size, size)
        ok, data = pixbuf.save_to_bufferv("png", [], [])
        if ok:
            image = QImage()
            image.loadFromData(bytes(data), "PNG")
            if not image.isNull():
                return image
    except Exception:
        pass

    if shutil.which("rsvg-convert"):
        data = subprocess.run(
            ["rsvg-convert", "-w", str(size), "-h", str(size), str(ICON)],
            capture_output=True, check=True).stdout
        image = QImage()
        image.loadFromData(data, "PNG")
        if not image.isNull():
            return image

    raise SystemExit(
        "FEHLER: Das Symbol ließ sich nicht rendern.\n"
        "        Abhilfe: sudo apt install gir1.2-gdkpixbuf-2.0 librsvg2-bin")


def build() -> QImage:
    img = QImage(W, H, QImage.Format.Format_ARGB32)
    p = QPainter(img)
    p.setRenderHint(QPainter.RenderHint.Antialiasing)
    p.setRenderHint(QPainter.RenderHint.SmoothPixmapTransform)
    p.setRenderHint(QPainter.RenderHint.TextAntialiasing)

    # --- Untergrund: leichter Verlauf, damit die Karte nicht flach wirkt ---
    grad = QLinearGradient(0, 0, 0, H)
    grad.setColorAt(0.0, BG_TOP)
    grad.setColorAt(1.0, BG_BOTTOM)
    p.fillRect(0, 0, W, H, QBrush(grad))

    # --- Linke Spalte: Symbol, Name, Beschreibung ---
    x = 68
    icon_px = 108
    p.drawImage(x, 74, _icon(icon_px))

    p.setPen(QPen(TITLE))
    p.setFont(_font(46, bold=True))
    p.drawText(QRectF(x + icon_px + 26, 74, 420, icon_px),
               int(Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter),
               "Inkpot")

    p.setFont(_font(21, bold=True))
    p.setPen(QPen(TITLE))
    p.drawText(QRectF(x, 214, 520, 90), int(Qt.TextFlag.TextWordWrap),
               "A paint program for Linux where\neverything stays an object")

    p.setFont(_font(15))
    p.setPen(QPen(BODY))
    # Die Karte ist englisch: sie wird international geteilt, während die
    # README beide Sprachen führt.
    # Die Zeilen müssen links vom Bildausschnitt bleiben (der beginnt bei
    # W - 600 - 56 - 16 = 608) — daher bewusst kurz gehalten.
    bullets = [
        "Brush · Line · Shapes · Bucket fill · Eraser · Text",
        "Move, turn, scale, recolour — at any time",
        "Eraser and bucket work across objects",
        "Object list · Undo · opens HEIC from phones",
    ]
    by = 318
    for line in bullets:
        p.setPen(QPen(ACCENT))
        p.setFont(_font(15, bold=True))
        p.drawText(x, by, "▸")
        p.setPen(QPen(BODY))
        p.setFont(_font(15))
        p.drawText(x + 22, by, line)
        by += 32

    # Fußzeile
    p.setPen(QPen(QColor("#8E99B4")))
    p.setFont(_font(14, bold=True))
    p.drawText(x, H - 62, "Free software · GPL-3.0 · C++ and Qt 6")
    p.setPen(QPen(ACCENT))
    p.drawText(x, H - 36, "github.com/Neklor/Inkpot")

    # --- Rechte Seite: das Programmfenster ---
    shot = QImage(str(SHOT))
    if shot.isNull():
        raise SystemExit(
            f"FEHLER: {SHOT} fehlt oder ist nicht lesbar.\n"
            "        Bildschirmfoto des Programmfensters dort ablegen.")

    panel_w = 600
    scaled = shot.scaledToWidth(panel_w, Qt.TransformationMode.SmoothTransformation)
    if scaled.height() > H - 120:
        scaled = shot.scaledToHeight(H - 120, Qt.TransformationMode.SmoothTransformation)
    px = W - scaled.width() - 56
    py = (H - scaled.height()) // 2

    # Dunkle Unterlage mit hellem Rand: hebt das Fenster vom Verlauf ab.
    frame = QRectF(px - 16, py - 16, scaled.width() + 32, scaled.height() + 32)
    p.setPen(Qt.PenStyle.NoPen)
    p.setBrush(QBrush(PANEL))
    p.drawRoundedRect(frame, 10, 10)
    p.drawImage(px, py, scaled)
    p.setPen(QPen(PANEL_EDGE, 2))
    p.setBrush(Qt.BrushStyle.NoBrush)
    p.drawRoundedRect(frame, 10, 10)

    p.end()
    return img


def main() -> None:
    # Referenz festhalten: ohne sie räumt Python die Anwendung sofort wieder
    # ab, und der erste Zugriff auf die Schriftdatenbank stürzt ab.
    _app = QGuiApplication([])   # noqa: F841 — für die Schriftdatenbank
    OUT.parent.mkdir(parents=True, exist_ok=True)
    if not build().save(str(OUT)):
        raise SystemExit(f"FEHLER: {OUT} konnte nicht geschrieben werden.")
    kb = OUT.stat().st_size / 1024
    print(f"Geschrieben: {OUT}  ({W}×{H}, {kb:.0f} KiB)")
    if kb > 1024:
        print("WARNUNG: über 1 MB — GitHub lehnt das ab.")
    print("Hochladen unter: Settings → General → Social preview")


if __name__ == "__main__":
    main()
