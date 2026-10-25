# Inkpot

Ein einfaches, objektbasiertes Malprogramm für Linux — C++ mit Qt 6.

Alles, was auf der Arbeitsfläche entsteht, ist ein eigenständiges **Objekt**:
Pinselstriche und eingefügte Bilder lassen sich auswählen, verschieben und
wieder löschen. Gespeichert wird im Projektformat `*.inkpot`, das diese
Objekte erhält; der Export erzeugt ein flaches Bild.

## Stand

**Meilenstein 1 — Grundlegendes Programm**

* Datei-Menü: Neu (Standardgrößen und benutzerdefiniert), Öffnen, Speichern,
  Speichern unter, Exportieren, In die Zwischenablage kopieren, Einfügen,
  Drucken, Beenden
* Arbeitsfläche mit acht Anfasspunkten; Größe jederzeit in der Fußzeile
  ablesbar und dort direkt eingebbar, mit Schalter zum Festhalten des
  Seitenverhältnisses und Anzeige des Verhältnisses (z. B. 4:3, 16:9)
* Objektfähigkeit: Auswählen, Verschieben, Mehrfachauswahl per Gummiband,
  Löschen

## Bauen

Voraussetzungen (Debian/Ubuntu/Mint):

```bash
sudo apt install build-essential cmake ninja-build qt6-base-dev qt6-base-dev-tools libgl1-mesa-dev
```

Bauen und starten:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/inkpot
```

## Bedienung

| Aktion | Eingabe |
| --- | --- |
| Auswahlwerkzeug | `S` |
| Pinsel | `B` |
| Auswahl löschen | `Entf` |
| Alles auswählen | `Strg+A` |
| Zoomen | `Strg+Mausrad`, `Strg++`, `Strg+-` |
| Originalgröße / Einpassen | `Strg+0` / `Strg+9` |
| Ansicht verschieben | Mittlere Maustaste ziehen |

## Projektstruktur

| Datei | Inhalt |
| --- | --- |
| `src/main.cpp` | Programmstart |
| `src/MainWindow.*` | Menüs, Werkzeugleiste, Fußzeile, Datei-Operationen |
| `src/CanvasScene.*` | Arbeitsfläche, Objektverwaltung, Projektformat |
| `src/CanvasView.*` | Ansicht, Werkzeuge, Zoom |
| `src/ResizeHandle.*` | Anfasspunkte der Arbeitsfläche |
| `src/StrokeItem.*` | Objekt: Pinselstrich |
| `src/ImageItem.*` | Objekt: Bild |
| `src/InkObject.h` | Gemeinsame Objekt-Schnittstelle |
| `src/NewCanvasDialog.*` | Dialog für benutzerdefinierte Größe |

## Lizenz

GNU General Public License v3.0 — siehe [LICENSE](LICENSE).
